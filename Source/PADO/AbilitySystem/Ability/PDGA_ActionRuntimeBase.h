#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PDGA_ActionRuntimeBase.generated.h"

class UPDAbilityTask_ActionTraceWindow;
class UPDAbilityTask_PlayActionMontage;
class APDWorldItemActor;
struct FPDActionTarget;
struct FPDActionTargetingContext;
struct FPDLoopingCueStruct;
struct FPDMontageHitLagConfigStruct;

/** Targeting, Hook 실행, 종료 정리를 공유하는 Action GA 런타임 기반이다. */
UCLASS(Abstract)
class PADO_API UPDGA_ActionRuntimeBase : public UPDGA_Base
{
	GENERATED_BODY()

public:
	UPDGA_ActionRuntimeBase();

	bool CanApplyMontageHitLag() const;
	bool ApplyMontageHitLag(const FPDMontageHitLagConfigStruct& HitLag);

	/** 현재 몽타주의 Socket Trace NotifyState가 호출하는 정확한 Ability 진입점이다. */
	void NotifySocketTraceWindowBegin();
	void NotifySocketTraceWindowTick();
	void NotifySocketTraceWindowEnd();

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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

	/**
	 * 현재 ActionTargeting으로 대상을 다시 수집해 OnExecute를 한 번 실행한다.
	 * 대상을 먼저 모으므로 OnExecuteStart가 이번 발이 멈춘 곳을 받는다.
	 */
	void ExecutePulse();

	/**
	 * 새 실행 구간을 열고, 열렸다면 OnExecuteStart Hook을 한 번 실행한다.
	 * 대상을 시간에 걸쳐 모으는 TraceWindow가 쓴다. 구간이 열리는 순간에는
	 * 아직 휘두른 결과가 없으므로 이번 발 결과 없이 실행한다.
	 */
	bool BeginExecutionWindow();

	/** Single/Channel이 각자의 규칙으로 새 실행 구간을 받을지 결정한다. */
	virtual bool TryBeginExecutionWindow() PURE_VIRTUAL(
		UPDGA_ActionRuntimeBase::TryBeginExecutionWindow,
		return false;);

	/** 충전 입력처럼 Press에서는 활성 상태만 만들고 실제 Commit을 미룰 때 사용한다. */
	virtual bool ShouldDeferActionExecutionStart() const;
	bool BeginActionExecution();
	bool HasActionExecutionStarted() const;

	/** 대상이 없는 시점의 Hook을 소스 자신에게 한 번 실행한다. Hook이 없으면 성공이다. */
	bool ExecuteSourceHook(
		FGameplayTag HookTag,
		const FHitResult* ShotResult = nullptr);

	void FinishAction(bool bWasCancelled, bool bRunCompleteHook);
	bool IsFinishingAction() const;

	/**
	 * 실행 구간 동안 유지할 Looping Cue 설정이다. 쓰지 않는 수명주기는 nullptr를 준다.
	 * 설정은 Definition이 소유하지만 Cue의 수명은 이 GA가 쥔다.
	 */
	virtual const FPDLoopingCueStruct* GetLoopingCueConfig() const;

	/** 현재 공격 몽타주의 재생과 일시적인 재생률 변경을 함께 소유한다. */
	UPROPERTY(Transient)
	TObjectPtr<UPDAbilityTask_PlayActionMontage> ActiveMontageTask;

	FPDActionTargetingContext BuildTargetingContext() const;

	/** 이번 활성화의 원본 아이템이다. 아이템이 아닌 Source면 nullptr다. */
	APDWorldItemActor* GetActiveSourceItem() const;

private:
	friend class UPDAbilityTask_ActionTraceWindow;

	/**
	 * 열린 실행 구간의 OnExecuteStart Hook을 실행한다. 실행 구간에 진입한 모든
	 * 경로가 이 함수를 지난다. 필수 Fragment가 실패하면 Action을 끝낸다.
	 */
	bool RunExecuteStartHook(const FHitResult* ShotResult);
	bool ExecuteTargets(const TArray<FPDActionTarget>& Targets);
	APDWorldItemActor* ResolveSourceItem(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo) const;
	void StartLoopingCue();
	void StopLoopingCue();

	TWeakObjectPtr<AActor> ActivationTarget;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;
	bool bFinishingAction = false;
	bool bActionExecutionStarted = false;
	bool bFirstHitHookExecuted = false;
	/** 현재 붙어 있는 Looping Cue다. 비어 있으면 붙은 Cue가 없다. */
	FGameplayTag ActiveLoopingCueTag;
	TWeakObjectPtr<APDWorldItemActor> ActiveSourceItem;

	UPROPERTY(Transient)
	TObjectPtr<UPDAbilityTask_ActionTraceWindow> ActiveTraceTask;
};
