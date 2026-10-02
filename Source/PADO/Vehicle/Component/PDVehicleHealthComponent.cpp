#include "PADO/Vehicle/Component/PDVehicleHealthComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Character/PDHealthComponent.h"
#include "PADO/Vehicle/Component/PDVehicleOccupancyComponent.h"
#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDVehicleHealth, Log, All);

UPDVehicleHealthComponent::UPDVehicleHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPDVehicleHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	UAbilitySystemComponent* AbilitySystem = FindAbilitySystem();
	const UPDHealthAttributeSet* Set =
		AbilitySystem ? AbilitySystem->GetSet<UPDHealthAttributeSet>() : nullptr;
	if (!Owner || !Set)
	{
		UE_LOG(
			LogPDVehicleHealth,
			Warning,
			TEXT("%s: 체력 Attribute Set을 가진 ASC가 없어 피해를 받지 않는다."),
			*GetNameSafe(Owner));
		return;
	}

	if (Owner->HasAuthority())
	{
		// 최대 체력을 먼저 바꾼다. 체력은 최대 체력 안으로 잘린다.
		AbilitySystem->SetNumericAttributeBase(
			UPDHealthAttributeSet::GetMaxHealthAttribute(),
			MaxHealth);
		AbilitySystem->SetNumericAttributeBase(
			UPDHealthAttributeSet::GetHealthAttribute(),
			MaxHealth);

		HealthSet = Set;
		DamagedHandle = Set->OnDamaged.AddUObject(this, &UPDVehicleHealthComponent::HandleDamaged);
	}

	// 파괴 상태가 BeginPlay보다 먼저 도착했으면(늦게 접속한 머신) 여기서 적용한다.
	ApplyDestruction();
}

void UPDVehicleHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UPDHealthAttributeSet* Set = HealthSet.Get())
	{
		Set->OnDamaged.Remove(DamagedHandle);
	}
	DamagedHandle.Reset();
	HealthSet.Reset();

	Super::EndPlay(EndPlayReason);
}

void UPDVehicleHealthComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDVehicleHealthComponent, Destruction);
}

void UPDVehicleHealthComponent::OnRep_Destruction()
{
	// BeginPlay 전이면 Blueprint가 아직 알림에 바인딩하지 않았다. BeginPlay가 적용한다.
	if (HasBegunPlay())
	{
		ApplyDestruction();
	}
}

void UPDVehicleHealthComponent::HandleDamaged(
	AActor* Instigator,
	AActor* EffectCauser,
	const FGameplayEffectSpec& EffectSpec,
	float Damage,
	float OldHealth,
	float NewHealth)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || Destruction.bDestroyed || NewHealth > 0.0f)
	{
		return;
	}

	Destruction.bDestroyed = true;
	Destruction.Instigator = Instigator;
	Destruction.EffectCauser = EffectCauser;
	Owner->ForceNetUpdate();

	ApplyDestruction();
	KillOccupants();
}

void UPDVehicleHealthComponent::ApplyDestruction()
{
	if (bDestructionApplied || !Destruction.bDestroyed)
	{
		return;
	}

	bDestructionApplied = true;

	// 파괴된 탈것은 더 다치지 않는다. 죽은 몸과 같은 규칙이다(UPDHealthAttributeSet).
	// 탈것의 ASC는 탈것과 수명이 같아 떼지 않는다.
	if (UAbilitySystemComponent* AbilitySystem = FindAbilitySystem())
	{
		AbilitySystem->AddLooseGameplayTag(TAG_PD_State_Dead);
	}

	OnVehicleDestroyed.Broadcast();
}

void UPDVehicleHealthComponent::KillOccupants() const
{
	const AActor* Owner = GetOwner();
	const UPDVehicleOccupancyComponent* Occupancy =
		Owner ? Owner->FindComponentByClass<UPDVehicleOccupancyComponent>() : nullptr;
	if (!Occupancy)
	{
		return;
	}

	// 사망 처리가 몸을 내리게 하면서 좌석이 비므로 먼저 모은다.
	TArray<APDCharacterBase*> Occupants;
	for (const UPDVehicleSeatComponent* Seat : Occupancy->GetSeats())
	{
		if (APDCharacterBase* Occupant = Occupancy->GetSeatOccupant(Seat))
		{
			Occupants.Add(Occupant);
		}
	}

	for (APDCharacterBase* Occupant : Occupants)
	{
		if (UPDHealthComponent* Health = Occupant->GetHealthComponent())
		{
			Health->Kill(Destruction.Instigator, Destruction.EffectCauser);
		}
	}
}

UAbilitySystemComponent* UPDVehicleHealthComponent::FindAbilitySystem() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}
