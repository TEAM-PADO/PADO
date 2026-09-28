#include "PADO/Item/Component/PDWeaponMagazineComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/Trait/PDItemMagazineTrait.h"

UPDWeaponMagazineComponent::UPDWeaponMagazineComponent()
{
	SetIsReplicatedByDefault(true);
}

bool UPDWeaponMagazineComponent::InitializeMagazine(bool bResetToFull)
{
	AActor* Owner = GetOwner();
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(Owner);
	const UPDItemDefinition* Definition = Item ? Item->GetItemDefinition() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !Definition)
	{
		return false;
	}
	const UPDItemMagazineTrait* MagazineTrait =
		Definition->FindTrait<UPDItemMagazineTrait>();
	if (!MagazineTrait)
	{
		CancelReload();
		CurrentMagazineAmmo = 0;
		bMagazineInitialized = false;
		InitializedDefinition.Reset();
		BroadcastMagazineChanged();
		ForceOwnerNetUpdate();
		return true;
	}

	const bool bDefinitionChanged =
		InitializedDefinition.Get() != Definition;
	if (bResetToFull || !bMagazineInitialized || bDefinitionChanged)
	{
		CancelReload();
		CurrentMagazineAmmo = MagazineTrait->Capacity;
	}
	else
	{
		CurrentMagazineAmmo = FMath::Clamp(
			CurrentMagazineAmmo,
			0,
			MagazineTrait->Capacity);
	}

	bMagazineInitialized = true;
	InitializedDefinition = Definition;
	BroadcastMagazineChanged();
	ForceOwnerNetUpdate();
	return true;
}

bool UPDWeaponMagazineComponent::CanConsumeRoundWithReplicatedState() const
{
	// bMagazineInitialized는 서버 전용 플래그다. 클라이언트는 Trait 존재와
	// 복제된 탄약·재장전 상태로 같은 결론에 도달한다.
	const AActor* Owner = GetOwner();
	const bool bInitialized =
		Owner && Owner->HasAuthority() ? bMagazineInitialized : true;
	return bInitialized && ResolveMagazineTrait() != nullptr && !bIsReloading &&
		!bReloadRequested && GetCurrentMagazineAmmo() > 0;
}

int32 UPDWeaponMagazineComponent::GetCurrentMagazineAmmo() const
{
	return FMath::Max(0, CurrentMagazineAmmo - GetUnprocessedLocalShotCount());
}

void UPDWeaponMagazineComponent::BeginLocalShotSession(
	FGameplayAbilitySpecHandle AbilityHandle,
	int32 LastShotIndex)
{
	const AActor* Owner = GetOwner();
	if (!Owner || Owner->HasAuthority())
	{
		return;
	}

	LocalShot.AbilityHandle = AbilityHandle;
	LocalShot.ShotIndex = FMath::Max(0, LastShotIndex);
	BroadcastMagazineChanged();
}

void UPDWeaponMagazineComponent::EndLocalShotSession(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (LocalShot.AbilityHandle != AbilityHandle)
	{
		return;
	}

	// 무기를 놓은 뒤 서버는 남은 발을 버린다. 그 발을 계속 빼고 보여 주면
	// 다시 주웠을 때 탄약이 모자란 것으로 보인다.
	LocalShot = FPDMagazineShotMarkStruct();
	BroadcastMagazineChanged();
}

void UPDWeaponMagazineComponent::RecordLocalShot(
	FGameplayAbilitySpecHandle AbilityHandle,
	int32 ShotIndex)
{
	const AActor* Owner = GetOwner();
	if (!Owner || Owner->HasAuthority())
	{
		return;
	}

	LocalShot.AbilityHandle = AbilityHandle;
	LocalShot.ShotIndex = ShotIndex;
	BroadcastMagazineChanged();
}

void UPDWeaponMagazineComponent::RecordProcessedShot(
	FGameplayAbilitySpecHandle AbilityHandle,
	int32 ShotIndex)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	ProcessedShot.AbilityHandle = AbilityHandle;
	ProcessedShot.ShotIndex = ShotIndex;
}

int32 UPDWeaponMagazineComponent::GetUnprocessedLocalShotCount() const
{
	const AActor* Owner = GetOwner();
	if (!Owner || Owner->HasAuthority() || !LocalShot.AbilityHandle.IsValid())
	{
		return 0;
	}

	// 처리 기록이 다른 Spec 것이면 이번 Spec의 발은 아직 하나도 처리되지 않았다.
	const int32 ProcessedIndex =
		ProcessedShot.AbilityHandle == LocalShot.AbilityHandle
			? ProcessedShot.ShotIndex
			: 0;
	return FMath::Max(0, LocalShot.ShotIndex - ProcessedIndex);
}

bool UPDWeaponMagazineComponent::CanRequestReload() const
{
	return ResolveMagazineTrait() != nullptr && !bIsReloading &&
		!bReloadRequested && GetCurrentMagazineAmmo() < GetMagazineCapacity();
}

void UPDWeaponMagazineComponent::MarkReloadRequested()
{
	bReloadRequested = true;
}

void UPDWeaponMagazineComponent::ClearReloadRequest()
{
	bReloadRequested = false;
}

void UPDWeaponMagazineComponent::ResolveReloadRequest()
{
	// 몽타주가 없는 재장전은 재장전 상태 없이 탄창만 찬다. 가득 찬 탄창이 답이다.
	// 몽타주 재장전은 OnRep_ReloadState가, 거부는 Held Item Component가 푼다.
	if (bIsReloading || GetCurrentMagazineAmmo() >= GetMagazineCapacity())
	{
		ClearReloadRequest();
	}
}

bool UPDWeaponMagazineComponent::CanConsumeRound(FString& OutError) const
{
	OutError.Reset();
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		OutError = TEXT("탄약은 서버에서만 소비할 수 있습니다.");
		return false;
	}

	if (!bMagazineInitialized || !ResolveMagazineTrait())
	{
		OutError = TEXT("Item Magazine이 유효한 Definition으로 초기화되지 않았습니다.");
		return false;
	}

	if (bIsReloading)
	{
		OutError = TEXT("재장전 중에는 발사할 수 없습니다.");
		return false;
	}

	if (CurrentMagazineAmmo <= 0)
	{
		OutError = TEXT("탄창이 비어 있습니다.");
		return false;
	}

	return true;
}

bool UPDWeaponMagazineComponent::TryConsumeRound()
{
	FString Error;
	if (!CanConsumeRound(Error))
	{
		return false;
	}

	--CurrentMagazineAmmo;
	BroadcastMagazineChanged();
	ForceOwnerNetUpdate();
	return true;
}

bool UPDWeaponMagazineComponent::TryStartReload()
{
	AActor* Owner = GetOwner();
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(Owner);
	const UPDItemMagazineTrait* MagazineTrait = ResolveMagazineTrait();
	UWorld* World = GetWorld();
	if (!Owner || !Owner->HasAuthority() || !Item || !MagazineTrait || !World ||
		!bMagazineInitialized || bIsReloading ||
		Item->GetItemState() != EPDWorldItemState::Held ||
		CurrentMagazineAmmo >= MagazineTrait->Capacity)
	{
		return false;
	}

	// 몽타주를 안 넣었으면 대기 없이 바로 채운다. 애니메이션이 아직 없는
	// 무기도 재장전은 되게 해 둔다.
	if (!MagazineTrait->ReloadMontage)
	{
		CurrentMagazineAmmo = MagazineTrait->Capacity;
		BroadcastMagazineChanged();
		ForceOwnerNetUpdate();
		return true;
	}

	bIsReloading = true;
	ReloadEndServerTime = GetSynchronizedWorldTime() +
		MagazineTrait->GetReloadCompleteTime() / MagazineTrait->ReloadSpeed;
	if (!PlayReloadMontage())
	{
		// Holder가 몽타주를 재생할 수 없으면 재장전을 시작하지 않는다.
		// 노티파이가 안 오므로 상태만 켜두면 영원히 끝나지 않는다.
		bIsReloading = false;
		ReloadEndServerTime = 0.0f;
		return false;
	}

	BroadcastReloadStateChanged();
	ForceOwnerNetUpdate();
	return true;
}

bool UPDWeaponMagazineComponent::NotifyReloadComplete()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !bIsReloading)
	{
		return false;
	}

	CompleteReload();
	return true;
}

bool UPDWeaponMagazineComponent::CancelReload()
{
	StopReloadMontage();

	if (!bIsReloading && ReloadEndServerTime <= 0.0f)
	{
		return false;
	}

	bIsReloading = false;
	ReloadEndServerTime = 0.0f;
	++ReloadCancelCounter;
	ObservedReloadCancelCounter = ReloadCancelCounter;
	BroadcastReloadStateChanged();
	ForceOwnerNetUpdate();
	return true;
}

int32 UPDWeaponMagazineComponent::GetMagazineCapacity() const
{
	const UPDItemMagazineTrait* MagazineTrait = ResolveMagazineTrait();
	return MagazineTrait ? MagazineTrait->Capacity : 0;
}

float UPDWeaponMagazineComponent::GetReloadRemainingTime() const
{
	return bIsReloading
		? FMath::Max(0.0f, ReloadEndServerTime - GetSynchronizedWorldTime())
		: 0.0f;
}

void UPDWeaponMagazineComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	CancelReload();
	Super::EndPlay(EndPlayReason);
}

void UPDWeaponMagazineComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(
		UPDWeaponMagazineComponent,
		CurrentMagazineAmmo,
		COND_OwnerOnly);
	DOREPLIFETIME(UPDWeaponMagazineComponent, bIsReloading);
	DOREPLIFETIME(UPDWeaponMagazineComponent, ReloadEndServerTime);
	DOREPLIFETIME(UPDWeaponMagazineComponent, ReloadCancelCounter);
	DOREPLIFETIME_CONDITION(
		UPDWeaponMagazineComponent,
		ProcessedShot,
		COND_OwnerOnly);
}

void UPDWeaponMagazineComponent::OnRep_CurrentMagazineAmmo()
{
	ResolveReloadRequest();
	BroadcastMagazineChanged();
}

void UPDWeaponMagazineComponent::OnRep_ProcessedShot()
{
	ResolveReloadRequest();
	BroadcastMagazineChanged();
}

void UPDWeaponMagazineComponent::OnRep_ReloadState()
{
	// 탄약은 서버가 정하고, 클라이언트는 같은 몽타주를 연출로만 맞춘다.
	// 완료는 몽타주를 끝까지 두고, 취소일 때만 끊는다.
	if (ObservedReloadCancelCounter != ReloadCancelCounter)
	{
		ObservedReloadCancelCounter = ReloadCancelCounter;
		StopReloadMontage();
	}
	else if (bIsReloading)
	{
		PlayReloadMontage();
	}

	// 서버가 재장전을 시작했거나, 시작한 뒤 바로 취소했다. 어느 쪽이든 요청에 대한
	// 답이다. 요청은 재장전 중이 아닐 때만 보내므로 이전 재장전의 소식일 수 없다.
	ClearReloadRequest();
	BroadcastReloadStateChanged();
}

void UPDWeaponMagazineComponent::OnReloadMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (ActiveReloadMontage.Get() != Montage)
	{
		return;
	}

	ActiveReloadMontage.Reset();

	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !bIsReloading)
	{
		return;
	}

	if (bInterrupted)
	{
		// 무기 교체나 피격으로 끊겼으면 채우지 않는다.
		CancelReload();
		return;
	}

	// 노티파이를 아직 안 찍은 몽타주는 끝까지 재생된 시점에 채운다.
	CompleteReload();
}

const UPDItemMagazineTrait*
UPDWeaponMagazineComponent::ResolveMagazineTrait() const
{
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(GetOwner());
	const UPDItemDefinition* Definition = Item ? Item->GetItemDefinition() : nullptr;
	return Definition ? Definition->FindTrait<UPDItemMagazineTrait>() : nullptr;
}

float UPDWeaponMagazineComponent::GetSynchronizedWorldTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const AGameStateBase* GameState = World->GetGameState();
	return GameState
		? GameState->GetServerWorldTimeSeconds()
		: World->GetTimeSeconds();
}

UAnimInstance* UPDWeaponMagazineComponent::ResolveHolderAnimInstance() const
{
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(GetOwner());
	const ACharacter* Holder =
		Item ? Cast<ACharacter>(Item->GetHolder()) : nullptr;
	const USkeletalMeshComponent* Mesh = Holder ? Holder->GetMesh() : nullptr;
	return Mesh ? Mesh->GetAnimInstance() : nullptr;
}

bool UPDWeaponMagazineComponent::PlayReloadMontage()
{
	const UPDItemMagazineTrait* MagazineTrait = ResolveMagazineTrait();
	UAnimMontage* Montage = MagazineTrait ? MagazineTrait->ReloadMontage : nullptr;
	UAnimInstance* AnimInstance = ResolveHolderAnimInstance();
	if (!Montage || !AnimInstance)
	{
		return false;
	}

	if (AnimInstance->Montage_Play(Montage, MagazineTrait->ReloadSpeed) <= 0.0f)
	{
		return false;
	}

	ActiveReloadMontage = Montage;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(
		this,
		&UPDWeaponMagazineComponent::OnReloadMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	return true;
}

void UPDWeaponMagazineComponent::StopReloadMontage()
{
	UAnimMontage* Montage = ActiveReloadMontage.Get();
	ActiveReloadMontage.Reset();
	if (!Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = ResolveHolderAnimInstance())
	{
		if (AnimInstance->Montage_IsPlaying(Montage))
		{
			AnimInstance->Montage_Stop(Montage->BlendOut.GetBlendTime(), Montage);
		}
	}
}

void UPDWeaponMagazineComponent::CompleteReload()
{
	AActor* Owner = GetOwner();
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(Owner);
	const UPDItemMagazineTrait* MagazineTrait = ResolveMagazineTrait();
	if (!Owner || !Owner->HasAuthority() || !Item || !MagazineTrait ||
		Item->GetItemState() != EPDWorldItemState::Held)
	{
		CancelReload();
		return;
	}

	bIsReloading = false;
	ReloadEndServerTime = 0.0f;
	CurrentMagazineAmmo = MagazineTrait->Capacity;
	BroadcastMagazineChanged();
	BroadcastReloadStateChanged();
	ForceOwnerNetUpdate();
}

void UPDWeaponMagazineComponent::BroadcastMagazineChanged()
{
	OnMagazineChanged.Broadcast(GetCurrentMagazineAmmo(), GetMagazineCapacity());
}

void UPDWeaponMagazineComponent::BroadcastReloadStateChanged()
{
	OnReloadStateChanged.Broadcast(bIsReloading, ReloadEndServerTime);
}

void UPDWeaponMagazineComponent::ForceOwnerNetUpdate() const
{
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}
