#include "PADO/Character/PDHealthComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Character/Struct/PDLifeStateChangedMessage.h"
#include "PADO/Character/Tag/PDCharacterGameplayTags.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"
#include "TimerManager.h"

namespace PDHealth
{
	/** 엔진 기본 충돌 프로필 중 래그돌용이다. */
	const FName RagdollCollisionProfile(TEXT("Ragdoll"));
}

UPDHealthComponent::UPDHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDHealthComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystem)
{
	if (AbilitySystem.Get() == InAbilitySystem)
	{
		return;
	}

	UninitializeFromAbilitySystem();
	if (!InAbilitySystem)
	{
		return;
	}

	AbilitySystem = InAbilitySystem;

	// 체력이 없는 ASC면 이 몸은 다치지 않는다. 체력을 쓰지 않는 캐릭터다.
	if (const UPDHealthAttributeSet* Set = InAbilitySystem->GetSet<UPDHealthAttributeSet>())
	{
		HealthSet = Set;
		DamagedHandle = Set->OnDamaged.AddUObject(this, &UPDHealthComponent::HandleDamaged);
	}

	// 클라이언트에서는 생명 상태가 ASC보다 먼저 도착할 수 있다.
	ApplyStateTags();
}

void UPDHealthComponent::UninitializeFromAbilitySystem()
{
	if (const UPDHealthAttributeSet* Set = HealthSet.Get())
	{
		Set->OnDamaged.Remove(DamagedHandle);
	}

	DamagedHandle.Reset();
	HealthSet.Reset();
	RemoveStateTags();
	AbilitySystem.Reset();
}

bool UPDHealthComponent::Revive()
{
	const AActor* Owner = GetOwner();
	UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get();
	const UPDHealthAttributeSet* Set = HealthSet.Get();
	if (!Owner || !Owner->HasAuthority() || LifeState.State != EPDLifeState::Downed ||
		!CurrentAbilitySystem || !Set)
	{
		return false;
	}

	CurrentAbilitySystem->SetNumericAttributeBase(
		UPDHealthAttributeSet::GetHealthAttribute(),
		Set->GetMaxHealth() * ReviveHealthRatio);
	SetLifeState(EPDLifeState::Alive, nullptr, nullptr);
	return true;
}

void UPDHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DownedTimerHandle);
	}

	if (SeatChangedHandle.IsValid())
	{
		if (const APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner()))
		{
			if (UPDVehicleOccupantComponent* Occupant = Character->GetVehicleOccupantComponent())
			{
				Occupant->OnSeatChanged.Remove(SeatChangedHandle);
			}
		}
		SeatChangedHandle.Reset();
	}

	// ASC는 몸보다 오래 살 수 있다. 이 몸이 붙인 상태 태그를 떼야 다음 몸이 살아 있는 상태로 시작한다.
	UninitializeFromAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

void UPDHealthComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDHealthComponent, LifeState);
}

void UPDHealthComponent::OnRep_LifeState(const FPDLifeStateStruct& PreviousLifeState)
{
	ApplyLifeState();
}

void UPDHealthComponent::HandleDamaged(
	AActor* Instigator,
	AActor* EffectCauser,
	const FGameplayEffectSpec& EffectSpec,
	float Damage,
	float OldHealth,
	float NewHealth)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (LifeState.State == EPDLifeState::Alive && NewHealth <= 0.0f)
	{
		SetLifeState(
			DownedDuration > 0.0f ? EPDLifeState::Downed : EPDLifeState::Dead,
			Instigator,
			EffectCauser);
	}
	else if (LifeState.State == EPDLifeState::Downed)
	{
		// 빈사 중에 다시 맞으면 사망한다.
		SetLifeState(EPDLifeState::Dead, Instigator, EffectCauser);
	}
}

void UPDHealthComponent::HandleDownedExpired()
{
	if (LifeState.State == EPDLifeState::Downed)
	{
		// 빈사로 만든 주체가 사망의 주체다.
		SetLifeState(EPDLifeState::Dead, LifeState.Instigator, LifeState.EffectCauser);
	}
}

void UPDHealthComponent::HandleSeatChanged()
{
	const APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	UPDVehicleOccupantComponent* Occupant =
		Character ? Character->GetVehicleOccupantComponent() : nullptr;
	if (!Occupant || Occupant->IsSeated())
	{
		return;
	}

	Occupant->OnSeatChanged.Remove(SeatChangedHandle);
	SeatChangedHandle.Reset();
	TryStartRagdoll();
}

void UPDHealthComponent::SetLifeState(
	EPDLifeState NewState,
	AActor* Instigator,
	AActor* EffectCauser)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || LifeState.State == NewState ||
		LifeState.State == EPDLifeState::Dead)
	{
		return;
	}

	LifeState.State = NewState;
	LifeState.Instigator = Instigator;
	LifeState.EffectCauser = EffectCauser;
	Owner->ForceNetUpdate();

	// 빈사 시간은 서버가 잰다.
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(DownedTimerHandle);
		if (NewState == EPDLifeState::Downed)
		{
			TimerManager.SetTimer(
				DownedTimerHandle,
				this,
				&UPDHealthComponent::HandleDownedExpired,
				DownedDuration,
				false);
		}
	}

	ApplyLifeState();
}

void UPDHealthComponent::ApplyLifeState()
{
	const EPDLifeState PreviousState = AppliedState;
	if (PreviousState == LifeState.State)
	{
		return;
	}

	AppliedState = LifeState.State;
	ApplyStateTags();

	APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	if (Character && PreviousState == EPDLifeState::Alive)
	{
		// 손으로 하던 일은 서버와 조종하는 머신이 각자 끊는다.
		if (Character->HasAuthority() || Character->IsLocallyControlled())
		{
			Character->InterruptHandActions();
		}

		// 빈사·사망한 몸은 탈것에 남지 않는다. 하차 위치와 속도는 탑승 상태로 복제된다.
		if (Character->HasAuthority())
		{
			if (UPDVehicleOccupantComponent* Occupant = Character->GetVehicleOccupantComponent())
			{
				Occupant->RequestExit();
			}
		}
	}

	if (Character && LifeState.State == EPDLifeState::Dead)
	{
		// 들고 있던 것은 손 자리에서 떨어뜨린다. 탈것에서 먼저 내렸으므로 차 안에 생기지 않는다.
		if (UPDHeldItemComponent* HeldItems = Character->GetHeldItemComponent();
			HeldItems && Character->HasAuthority())
		{
			if (const APDWorldItemActor* Item = HeldItems->GetHeldItem())
			{
				HeldItems->DropHeldItem(Item->GetActorTransform());
			}
		}

		TryStartRagdoll();
	}

	BroadcastLifeStateChanged(PreviousState);
}

void UPDHealthComponent::ApplyStateTags()
{
	FGameplayTagContainer DesiredTags;
	if (LifeState.State == EPDLifeState::Downed)
	{
		DesiredTags.AddTag(TAG_PD_State_Downed);
		DesiredTags.AddTag(TAG_PD_State_HandsBlocked);
	}
	else if (LifeState.State == EPDLifeState::Dead)
	{
		DesiredTags.AddTag(TAG_PD_State_Dead);
		DesiredTags.AddTag(TAG_PD_State_HandsBlocked);
	}

	UAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get();
	if (TaggedAbilitySystem.Get() == CurrentAbilitySystem && AppliedStateTags == DesiredTags)
	{
		return;
	}

	RemoveStateTags();
	if (!CurrentAbilitySystem || DesiredTags.IsEmpty())
	{
		return;
	}

	CurrentAbilitySystem->AddLooseGameplayTags(DesiredTags);
	TaggedAbilitySystem = CurrentAbilitySystem;
	AppliedStateTags = DesiredTags;
}

void UPDHealthComponent::RemoveStateTags()
{
	if (UAbilitySystemComponent* CurrentAbilitySystem = TaggedAbilitySystem.Get())
	{
		CurrentAbilitySystem->RemoveLooseGameplayTags(AppliedStateTags);
	}

	TaggedAbilitySystem.Reset();
	AppliedStateTags.Reset();
}

void UPDHealthComponent::TryStartRagdoll()
{
	APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	if (bRagdollStarted || !Character || LifeState.State != EPDLifeState::Dead)
	{
		return;
	}

	// 클라이언트에는 사망이 하차보다 먼저 도착할 수 있다. 몸이 좌석에서 내린 뒤에 시작한다.
	if (UPDVehicleOccupantComponent* Occupant = Character->GetVehicleOccupantComponent();
		Occupant && Occupant->IsSeated())
	{
		if (!SeatChangedHandle.IsValid())
		{
			SeatChangedHandle = Occupant->OnSeatChanged.AddUObject(
				this,
				&UPDHealthComponent::HandleSeatChanged);
		}
		return;
	}

	bRagdollStarted = true;

	// 쓰러지기 직전의 속도를 래그돌이 이어받는다. 달리던 차에서 내린 몸이면 차의 속도다.
	FVector InheritedVelocity = FVector::ZeroVector;
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		InheritedVelocity = Movement->Velocity;
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
		Movement->SetComponentTickEnabled(false);
	}

	if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 래그돌은 연출이라 머신마다 따로 계산한다. 화면이 없는 데디케이티드 서버는 하지 않는다.
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh || IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	Mesh->SetCollisionProfileName(PDHealth::RagdollCollisionProfile);
	Mesh->SetAllBodiesSimulatePhysics(true);
	Mesh->SetSimulatePhysics(true);
	Mesh->WakeAllRigidBodies();
	Mesh->bBlendPhysics = true;
	Mesh->SetAllPhysicsLinearVelocity(InheritedVelocity);
}

void UPDHealthComponent::BroadcastLifeStateChanged(EPDLifeState PreviousState)
{
	OnLifeStateChanged.Broadcast(PreviousState, LifeState.State);

	// 메시지 라우터는 GameInstance 서브시스템이라 GameInstance가 없는 World에서는 없다.
	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	FPDLifeStateChangedMessage Message;
	Message.Character = Cast<APDCharacterBase>(GetOwner());
	Message.PreviousState = PreviousState;
	Message.NewState = LifeState.State;
	Message.Instigator = LifeState.Instigator;
	Message.EffectCauser = LifeState.EffectCauser;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		TAG_PD_Message_Character_LifeStateChanged,
		Message);
}
