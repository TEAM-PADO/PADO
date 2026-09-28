#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"
#include "PADO/AbilitySystem/Struct/PDFireShotStruct.h"
#include "PDGA_FireAction.generated.h"

class UAbilitySystemComponent;
class UPDAbilityDefinition;
class UPDAbilityTask_FireLoop;
class UPDHeldItemComponent;
class UPDWeaponMagazineComponent;

/** 한 발을 쏘려 했을 때의 결과다. 발사 일정이 다음 행동을 정하는 데 쓴다. */
enum class EPDFireShotOutcome : uint8
{
	/** 발이 나갔다. */
	Fired,

	/** Block 태그로 지금은 쏠 수 없다. 방아쇠 상태는 유지하고 풀리면 이어서 쏜다. */
	Blocked,

	/** 탄약 소진, 재장전, 무기 해제로 쏠 수 없다. 이번 방아쇠 입력을 끝낸다. */
	Exhausted
};

/**
 * 방아쇠로 쏘는 무기의 Action이다. 무기를 들고 있는 동안 활성 상태를 유지하고,
 * 발사는 활성화가 아니라 이 활성화 안에서 일어난다.
 *
 * 조종하는 머신이 자기 시계로 발을 쏘고 판정, 연출, 반동을 즉시 실행한다. 발
 * 기록은 묶어서 서버로 보내고, 서버는 받은 판정을 다시 추적하지 않고 적용한 뒤
 * 관찰자에게 전달한다. 설계는 docs/FireActionSystem.md에 있다.
 */
UCLASS(Config = Game, meta = (DisplayName = "PDGA_FireAction"))
class PADO_API UPDGA_FireAction : public UPDGA_ActionRuntimeBase
{
	GENERATED_BODY()

public:
	UPDGA_FireAction();

	virtual bool UsesLocalTriggerInput() const override;

	/** 발사 일정이 부른다. 게이트를 통과하면 한 발을 판정하고 보여 주고 기록한다. */
	EPDFireShotOutcome TryFireLocalShot();

	/**
	 * 쌓인 발을 내보낸다. 클라이언트는 서버로 보내고, 서버는 처리한 발을
	 * 관찰자에게 전달한다. 보낼 발을 호출 사이에 남기지 않으므로 이 뒤에
	 * 보낸 재장전이나 드롭 요청보다 늦게 도착하는 발이 없다.
	 */
	void FlushShotBatch();

	/** 서버가 받은 발 묶음을 순서대로 처리한다. */
	void ProcessShotBatch(const FPDFireShotBatchStruct& Batch);

	/**
	 * 발 묶음의 연출 Fragment를 이 머신에서 실행한다. GA 인스턴스가 없는 관찰자도
	 * 부르므로 정적 함수다. Definition은 발을 쏜 Ability Source에서 얻는다.
	 */
	static void PresentShotBatch(
		UAbilitySystemComponent* ShooterAbilitySystem,
		const FPDFireShotBatchStruct& Batch);

	/**
	 * 명중을 인정하는 사수 핑의 상한(ms)이다. 넘으면 탄약 소비와 연출은 그대로
	 * 두고 대상에게 주는 결과만 적용하지 않는다. DefaultGame.ini에서 조정한다.
	 */
	UPROPERTY(Config)
	float MaxAcceptedPingMilliseconds = 250.0f;

protected:
	virtual void OnGiveAbility(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

	/**
	 * 활성화는 태그 조건을 보지 않는다. 막힌 상태로 무기를 들었을 때 활성화가
	 * 실패하면 다시 들기 전까지 쏠 수 없다. Block 태그는 발마다 본다.
	 */
	virtual bool DoesAbilitySatisfyTagRequirements(
		const UAbilitySystemComponent& AbilitySystemComponent,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** 발 간격은 Definition의 ShotInterval이 정한다. ActionCooldown은 쓰지 않는다. */
	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual bool TryBeginExecutionWindow() override;
	virtual bool GatesActivationOnPredictedState() const override;

private:
	EPDFireShotOutcome EvaluateShotGate() const;

	/** 서버에서 한 발의 결과 Fragment를 실행한다. 비용을 치르지 못한 발이면 false다. */
	bool ProcessShot(const FPDFireShotStruct& Shot, bool bAcceptHits);

	/** 사수의 서버 측정 핑이다. PlayerState가 없으면 0이다. */
	float ResolveShooterPingMilliseconds() const;

	static void PresentShot(
		UAbilitySystemComponent* ShooterAbilitySystem,
		UObject* SourceObject,
		const UPDAbilityDefinition& Definition,
		const FPDFireShotStruct& Shot,
		UPDGA_Base* Ability);

	void ResetShotBatches();

	UPROPERTY(Transient)
	TObjectPtr<UPDAbilityTask_FireLoop> FireLoop;

	/** 조종하는 클라이언트가 서버로 보낼 발이다. */
	UPROPERTY(Transient)
	FPDFireShotBatchStruct PendingShots;

	/** 서버가 처리해 관찰자에게 전달할 발이다. */
	UPROPERTY(Transient)
	FPDFireShotBatchStruct PendingRelay;

	TWeakObjectPtr<UPDHeldItemComponent> ShooterHeldItems;
	TWeakObjectPtr<UPDWeaponMagazineComponent> SourceMagazine;

	/**
	 * 이 Spec에서 마지막으로 쏜 발 번호다. 인스턴스는 Spec과 수명이 같으므로
	 * 활성화가 다시 열려도 이어서 센다. 탄약 보정이 Spec과 발 번호로 발을 가린다.
	 */
	int32 LastShotIndex = 0;
};
