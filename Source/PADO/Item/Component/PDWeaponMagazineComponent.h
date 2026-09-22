#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDWeaponMagazineComponent.generated.h"

class UPDItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FPDWeaponMagazineChangedSignature,
	int32,
	CurrentMagazineAmmo,
	int32,
	MagazineCapacity);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FPDWeaponReloadStateChangedSignature,
	bool,
	bIsReloading,
	float,
	ReloadEndServerTime);

/** 탄창 기능이 활성화된 Item Actor의 서버 권한 런타임 상태다. */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDWeaponMagazineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDWeaponMagazineComponent();

	/** Definition 교체 시에는 ResetToFull을 사용하고, BeginPlay 재진입은 상태를 보존한다. */
	bool InitializeMagazine(bool bResetToFull);

	bool CanConsumeRound(FString& OutError) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool TryConsumeRound();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool TryStartReload();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool CancelReload();

	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	int32 GetCurrentMagazineAmmo() const { return CurrentMagazineAmmo; }

	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	int32 GetMagazineCapacity() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	float GetReloadEndServerTime() const { return ReloadEndServerTime; }

	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	float GetReloadRemainingTime() const;

	UPROPERTY(BlueprintAssignable, Category = "PD|Item|Weapon")
	FPDWeaponMagazineChangedSignature OnMagazineChanged;

	UPROPERTY(BlueprintAssignable, Category = "PD|Item|Weapon")
	FPDWeaponReloadStateChangedSignature OnReloadStateChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentMagazineAmmo();

	UFUNCTION()
	void OnRep_ReloadState();

private:
	const UPDItemDefinition* ResolveMagazineDefinition() const;
	float GetSynchronizedWorldTime() const;
	void CompleteReload();
	void BroadcastMagazineChanged();
	void BroadcastReloadStateChanged();
	void ForceOwnerNetUpdate() const;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentMagazineAmmo)
	int32 CurrentMagazineAmmo = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ReloadState)
	bool bIsReloading = false;

	UPROPERTY(ReplicatedUsing = OnRep_ReloadState)
	float ReloadEndServerTime = 0.0f;

	bool bMagazineInitialized = false;
	TWeakObjectPtr<const UPDItemDefinition> InitializedDefinition;
	FTimerHandle ReloadTimerHandle;
};
