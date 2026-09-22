#include "PADO/Item/Component/PDWeaponMagazineComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "TimerManager.h"

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
	if (!Definition->HasMagazine())
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
		CurrentMagazineAmmo = Definition->Magazine.Capacity;
	}
	else
	{
		CurrentMagazineAmmo = FMath::Clamp(
			CurrentMagazineAmmo,
			0,
			Definition->Magazine.Capacity);
	}

	bMagazineInitialized = true;
	InitializedDefinition = Definition;
	BroadcastMagazineChanged();
	ForceOwnerNetUpdate();
	return true;
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

	if (!bMagazineInitialized || !ResolveMagazineDefinition())
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
	const UPDItemDefinition* Definition = ResolveMagazineDefinition();
	UWorld* World = GetWorld();
	if (!Owner || !Owner->HasAuthority() || !Item || !Definition || !World ||
		!bMagazineInitialized || bIsReloading ||
		Item->GetItemState() != EPDWorldItemState::Held ||
		CurrentMagazineAmmo >= Definition->Magazine.Capacity)
	{
		return false;
	}

	bIsReloading = true;
	ReloadEndServerTime = GetSynchronizedWorldTime() + Definition->Magazine.ReloadDuration;
	World->GetTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&UPDWeaponMagazineComponent::CompleteReload,
		Definition->Magazine.ReloadDuration,
		false);
	BroadcastReloadStateChanged();
	ForceOwnerNetUpdate();
	return true;
}

bool UPDWeaponMagazineComponent::CancelReload()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}
	else
	{
		ReloadTimerHandle.Invalidate();
	}

	if (!bIsReloading && ReloadEndServerTime <= 0.0f)
	{
		return false;
	}

	bIsReloading = false;
	ReloadEndServerTime = 0.0f;
	BroadcastReloadStateChanged();
	ForceOwnerNetUpdate();
	return true;
}

int32 UPDWeaponMagazineComponent::GetMagazineCapacity() const
{
	const UPDItemDefinition* Definition = ResolveMagazineDefinition();
	return Definition ? Definition->Magazine.Capacity : 0;
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
}

void UPDWeaponMagazineComponent::OnRep_CurrentMagazineAmmo()
{
	BroadcastMagazineChanged();
}

void UPDWeaponMagazineComponent::OnRep_ReloadState()
{
	BroadcastReloadStateChanged();
}

const UPDItemDefinition*
UPDWeaponMagazineComponent::ResolveMagazineDefinition() const
{
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(GetOwner());
	const UPDItemDefinition* Definition = Item ? Item->GetItemDefinition() : nullptr;
	return Definition && Definition->HasMagazine() ? Definition : nullptr;
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

void UPDWeaponMagazineComponent::CompleteReload()
{
	AActor* Owner = GetOwner();
	const APDWorldItemActor* Item = Cast<APDWorldItemActor>(Owner);
	const UPDItemDefinition* Definition = ResolveMagazineDefinition();
	if (!Owner || !Owner->HasAuthority() || !Item || !Definition ||
		Item->GetItemState() != EPDWorldItemState::Held)
	{
		CancelReload();
		return;
	}

	ReloadTimerHandle.Invalidate();
	bIsReloading = false;
	ReloadEndServerTime = 0.0f;
	CurrentMagazineAmmo = Definition->Magazine.Capacity;
	BroadcastMagazineChanged();
	BroadcastReloadStateChanged();
	ForceOwnerNetUpdate();
}

void UPDWeaponMagazineComponent::BroadcastMagazineChanged()
{
	OnMagazineChanged.Broadcast(CurrentMagazineAmmo, GetMagazineCapacity());
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
