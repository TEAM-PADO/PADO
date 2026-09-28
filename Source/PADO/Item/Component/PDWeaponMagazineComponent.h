#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "PDWeaponMagazineComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class UPDItemDefinition;
class UPDItemMagazineTrait;

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

/**
 * Fire Action의 발 하나를 가리킨다. 발 번호는 Ability Spec마다 따로 세므로 Spec과
 * 함께 있어야 다른 줍기(다른 Spec)의 발과 섞이지 않는다.
 */
USTRUCT()
struct PADO_API FPDMagazineShotMarkStruct
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilityHandle;

	UPROPERTY()
	int32 ShotIndex = 0;
};

/** 탄창 기능이 활성화된 Item Actor의 서버 권한 런타임 상태다. */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDWeaponMagazineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDWeaponMagazineComponent();

	/** Definition 교체 시에는 ResetToFull을 사용하고, BeginPlay 재진입은 상태를 보존한다. */
	bool InitializeMagazine(bool bResetToFull);

	/**
	 * 복제된 상태만으로 판정한다. 서버와 소유 클라이언트가 같은 함수를 본다.
	 *
	 * 예측에는 클라이언트도 같은 판정이 필요하다. 조건을 따로 적으면 반드시
	 * 어긋나므로 판정은 여기 한 곳에만 둔다. 소유 클라이언트는 예측 탄약과
	 * 보내 둔 재장전 요청까지 본다.
	 */
	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	bool CanConsumeRoundWithReplicatedState() const;

	/**
	 * 소유 클라이언트가 Fire Action을 시작했다. 이 Spec의 발 번호를 이어서 센다.
	 * 이전 줍기에서 남은 기록이 새 발의 예측을 흐리지 않게 한다.
	 */
	void BeginLocalShotSession(
		FGameplayAbilitySpecHandle AbilityHandle,
		int32 LastShotIndex);

	/** 무기를 놓았다. 서버가 버린 발까지 모두 잊고 복제된 탄약을 그대로 본다. */
	void EndLocalShotSession(FGameplayAbilitySpecHandle AbilityHandle);

	/**
	 * 소유 클라이언트가 방금 쏜 발이다. 서버가 처리할 때까지 그만큼 탄약을 빼고
	 * 보여 주고 판정한다.
	 */
	void RecordLocalShot(FGameplayAbilitySpecHandle AbilityHandle, int32 ShotIndex);

	/**
	 * 서버가 발 기록 하나를 처리했다. 탄약을 썼는지와 무관하게 기록한다. 탄약과
	 * 같은 갱신으로 소유자에게 복제된다.
	 */
	void RecordProcessedShot(FGameplayAbilitySpecHandle AbilityHandle, int32 ShotIndex);

	/** 쏘고 서버가 아직 처리하지 않은 자기 발 수다. 서버에서는 언제나 0이다. */
	int32 GetUnprocessedLocalShotCount() const;

	/** 재장전을 요청할 수 있는가. 예측 탄약이 가득 차 있거나 이미 요청했으면 아니다. */
	bool CanRequestReload() const;

	/**
	 * 재장전을 요청했다. 서버의 재장전 상태가 오거나 거부될 때까지 발사를 막는다.
	 * 쏘던 중이어도 이 순간 스스로 멈춘다.
	 */
	void MarkReloadRequested();

	void ClearReloadRequest();

	bool IsReloadRequested() const { return bReloadRequested; }

	bool CanConsumeRound(FString& OutError) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool TryConsumeRound();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool TryStartReload();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool CancelReload();

	/**
	 * 재장전 몽타주의 Reload Complete 노티파이가 부른다. 서버에서만 유효하다.
	 * 몽타주가 없어 즉시 장전한 경우에는 거치지 않는다.
	 */
	bool NotifyReloadComplete();

	/**
	 * 이 머신이 보는 현재 탄약이다. 소유 클라이언트는 서버가 아직 처리하지 않은
	 * 자기 발만큼 뺀 예측 값을 본다. 서버에서는 실제 탄약이다.
	 */
	UFUNCTION(BlueprintPure, Category = "PD|Item|Weapon")
	int32 GetCurrentMagazineAmmo() const;

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

	UFUNCTION()
	void OnRep_ProcessedShot();

	/** 몽타주가 끝나거나 끊겼을 때 재장전 상태를 정리한다. */
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);

private:
	/** 탄창 Trait이 없으면 nullptr. Definition 교체를 따라가야 해서 매번 조회한다. */
	const UPDItemMagazineTrait* ResolveMagazineTrait() const;
	float GetSynchronizedWorldTime() const;

	/** Holder 캐릭터의 AnimInstance다. 몽타주를 재생할 수 없으면 nullptr. */
	UAnimInstance* ResolveHolderAnimInstance() const;
	bool PlayReloadMontage();
	void StopReloadMontage();

	void CompleteReload();

	/** 탄창이 찼으면 보내 둔 재장전 요청을 푼다. 몽타주 없는 재장전의 답이다. */
	void ResolveReloadRequest();

	void BroadcastMagazineChanged();
	void BroadcastReloadStateChanged();
	void ForceOwnerNetUpdate() const;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentMagazineAmmo)
	int32 CurrentMagazineAmmo = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ReloadState)
	bool bIsReloading = false;

	UPROPERTY(ReplicatedUsing = OnRep_ReloadState)
	float ReloadEndServerTime = 0.0f;

	/**
	 * 취소될 때만 증가한다. 정상 완료와 취소를 구분하려고 둔다. 완료는
	 * 노티파이 시점이라 몽타주가 아직 남아 있고, 그때 멈추면 클라이언트에서
	 * 재장전 동작이 중간에 잘린다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ReloadState)
	uint8 ReloadCancelCounter = 0;

	/**
	 * 서버가 마지막으로 처리한 발이다. 탄약과 같은 객체의 속성이라 같은 갱신으로
	 * 소유자에게 함께 도착한다. 소유 클라이언트는 이것으로 미처리 발을 센다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ProcessedShot)
	FPDMagazineShotMarkStruct ProcessedShot;

	/** 이 머신이 마지막으로 쏜 발이다. 소유 클라이언트에서만 쓰며 복제하지 않는다. */
	FPDMagazineShotMarkStruct LocalShot;

	/** 재장전을 요청하고 서버의 답을 기다리는 중인가. 복제하지 않는다. */
	bool bReloadRequested = false;

	bool bMagazineInitialized = false;
	TWeakObjectPtr<const UPDItemDefinition> InitializedDefinition;

	/** 재생 중인 재장전 몽타주다. 취소할 때 같은 몽타주만 멈추려고 들고 있다. */
	TWeakObjectPtr<UAnimMontage> ActiveReloadMontage;

	/** 이 인스턴스가 마지막으로 반영한 취소 카운터다. */
	uint8 ObservedReloadCancelCounter = 0;
};
