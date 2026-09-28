#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"
#include "PDGA_Action.generated.h"

class UPDAbilityTask_PlayActionMontage;

/**
 * Press 한 번에 OnExecute를 최대 한 번 실행하는 ServerOnly Action이다.
 * 몽타주가 없으면 즉시 실행하고, 있으면 자기 몽타주의 첫 Event 또는 TraceWindow를 받는다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "PDGA_Action"))
class PADO_API UPDGA_Action : public UPDGA_ActionRuntimeBase
{
	GENERATED_BODY()

public:
	UPDGA_Action();

	UFUNCTION()
	void HandleExecuteEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual bool TryBeginExecutionWindow() override;
	virtual bool ShouldDeferActionExecutionStart() const override;

private:
	void BeginExecutionSequence();

	bool bExecuteAttempted = false;
	bool bWaitingForInputRelease = false;
	double ChargeStartTimeSeconds = 0.0;

};
