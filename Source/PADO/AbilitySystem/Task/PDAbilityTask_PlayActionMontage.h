#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Struct/PDActionMontageConfigStruct.h"
#include "PDAbilityTask_PlayActionMontage.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
struct FPDMontageHitLagConfigStruct;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPDActionMontageExecuteDelegate,
	FGameplayEventData,
	Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDActionMontageTerminalDelegate);

/**
 * Action 몽타주 재생과 선택적인 Execute Event 대기를 한 수명주기로 묶는다.
 * Event 방식에서는 payload가 이 태스크의 Ability를 가리킬 때만 Execute를 전달한다.
 */
UCLASS()
class PADO_API UPDAbilityTask_PlayActionMontage : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UPDAbilityTask_PlayActionMontage* Create(
		UGameplayAbility* OwningAbility,
		const FPDActionMontageConfigStruct& MontageConfig,
		bool bInListenForExecuteEvent);

	UPROPERTY(BlueprintAssignable)
	FPDActionMontageExecuteDelegate OnExecute;

	UPROPERTY(BlueprintAssignable)
	FPDActionMontageTerminalDelegate OnCompleted;

	UPROPERTY(BlueprintAssignable)
	FPDActionMontageTerminalDelegate OnInterrupted;

	virtual void Activate() override;

	bool CanApplyHitLag() const;
	bool ApplyHitLag(const FPDMontageHitLagConfigStruct& HitLag);

protected:
	virtual void OnDestroy(bool bAbilityEnded) override;

private:
	UFUNCTION()
	void HandleGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	void FinishTask(bool bInterrupted);
	void RestoreHitLag();

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> EventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FGameplayAbilitySpecHandle AbilityHandle;
	float PlayRate = 1.0f;
	FName StartSection = NAME_None;
	FTimerHandle HitLagTimerHandle;
	uint32 HitLagGeneration = 0;
	bool bHitLagActive = false;
	bool bTerminal = false;
	bool bCleaningUp = false;
	bool bListenForExecuteEvent = true;
};
