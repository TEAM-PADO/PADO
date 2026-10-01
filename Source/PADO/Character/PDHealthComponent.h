#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PADO/Character/Enum/PDLifeState.h"
#include "PDHealthComponent.generated.h"

class UAbilitySystemComponent;
class UPDHealthAttributeSet;
struct FGameplayEffectSpec;

/** 생명 상태와 그 상태를 만든 주체다. 한 구조체로 복제해 도착 순서가 어긋나지 않게 한다. */
USTRUCT()
struct FPDLifeStateStruct
{
	GENERATED_BODY()

	UPROPERTY()
	EPDLifeState State = EPDLifeState::Alive;

	/** 이 상태로 만든 논리적 주체다(플레이어면 PlayerState). */
	UPROPERTY()
	TObjectPtr<AActor> Instigator = nullptr;

	/** 이 상태로 만든 물리적 원인이다(아이템·투사체). */
	UPROPERTY()
	TObjectPtr<AActor> EffectCauser = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FPDLifeStateChangedSignature,
	EPDLifeState,
	PreviousState,
	EPDLifeState,
	NewState);

/**
 * 몸의 생명 상태(살아 있음, 빈사, 사망)다. 몸이 연결된 ASC의 체력을 보고 서버가
 * 상태를 바꾸고, 그 상태를 복제해 모든 머신이 몸에 적용한다.
 *
 * 체력은 ASC가 가지므로(플레이어는 PlayerState) 몸보다 오래 산다. 생명 상태는
 * 몸의 것이라 새 몸은 살아 있는 상태로 시작한다. 이 몸이 ASC에 붙인 상태 태그는
 * 연결이 풀리거나 몸이 사라질 때 뗀다.
 *
 * - 빈사·사망: State.Downed/State.Dead와 State.HandsBlocked를 붙이고, 손 행동을
 *   끊고, 타고 있으면 내린다.
 * - 사망: 들고 있던 아이템을 바닥에 떨어뜨리고, 이동과 캡슐 충돌을 끄고 래그돌이 된다.
 * - 상태가 바뀌면 OnLifeStateChanged와 Gameplay Message
 *   `Message.Character.LifeStateChanged`로 알린다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDHealthComponent();

	/** 몸이 ASC에 연결될 때 캐릭터 베이스가 부른다. 그 ASC의 체력을 구독한다. */
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystem);

	/** 연결이 풀릴 때 캐릭터 베이스가 부른다. 이 몸이 붙인 상태 태그를 뗀다. */
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintPure, Category = "PD|Health")
	EPDLifeState GetLifeState() const { return LifeState.State; }

	UFUNCTION(BlueprintPure, Category = "PD|Health")
	bool IsAlive() const { return LifeState.State == EPDLifeState::Alive; }

	UFUNCTION(BlueprintPure, Category = "PD|Health")
	bool IsDead() const { return LifeState.State == EPDLifeState::Dead; }

	/** 서버에서 빈사인 몸을 살린다. 체력은 최대 체력의 ReviveHealthRatio만큼 채운다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Health")
	bool Revive();

	/** 이 머신에 생명 상태가 적용될 때 발생한다. */
	UPROPERTY(BlueprintAssignable, Category = "PD|Health")
	FPDLifeStateChangedSignature OnLifeStateChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LifeState(const FPDLifeStateStruct& PreviousLifeState);

	/**
	 * 빈사로 버티는 시간이다. 0이면 빈사 없이 체력이 0이 되는 순간 사망한다.
	 * 빈사 중 이동, 팀원이 살리는 상호작용, 빈사 애니메이션은 빈사를 켤 때 정한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Health", meta = (ClampMin = "0.0", Units = "s"))
	float DownedDuration = 0.0f;

	/** 빈사에서 살아날 때 채우는 체력이다. 최대 체력에 대한 비율이다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Health", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReviveHealthRatio = 0.3f;

private:
	void HandleDamaged(
		AActor* Instigator,
		AActor* EffectCauser,
		const FGameplayEffectSpec& EffectSpec,
		float Damage,
		float OldHealth,
		float NewHealth);
	void HandleDownedExpired();
	void HandleSeatChanged();

	/** 서버에서 상태를 바꾸고 이 머신에 적용한다. 사망은 되돌리지 않는다. */
	void SetLifeState(EPDLifeState NewState, AActor* Instigator, AActor* EffectCauser);

	/** 이 머신에 현재 상태를 적용한다. 같은 상태가 다시 와도 한 번만 적용한다. */
	void ApplyLifeState();
	void ApplyStateTags();
	void RemoveStateTags();
	void TryStartRagdoll();
	void BroadcastLifeStateChanged(EPDLifeState PreviousState);

	UPROPERTY(ReplicatedUsing = OnRep_LifeState)
	FPDLifeStateStruct LifeState;

	/** 이 머신에 적용한 상태다. 복제가 같은 상태로 다시 와도 한 번만 적용한다. */
	EPDLifeState AppliedState = EPDLifeState::Alive;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	TWeakObjectPtr<const UPDHealthAttributeSet> HealthSet;
	FDelegateHandle DamagedHandle;

	/** 상태 태그를 붙인 ASC와 태그다. 같은 태그를 다른 상태가 붙였을 수 있어 붙인 만큼만 뗀다. */
	TWeakObjectPtr<UAbilitySystemComponent> TaggedAbilitySystem;
	FGameplayTagContainer AppliedStateTags;

	FTimerHandle DownedTimerHandle;
	FDelegateHandle SeatChangedHandle;
	bool bRagdollStarted = false;
};
