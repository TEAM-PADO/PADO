#pragma once

#include "Abilities/GameplayAbilityTypes.h"
#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"
#include "PDGA_ChannelAction.generated.h"

class UPDAbilityTask_PlayActionMontage;
struct FPDLoopingCueStruct;

/** Press부터 Release까지 유지되고 각 Event 또는 TraceWindow마다 결과를 만드는 ServerOnly Action이다. */
UCLASS(Blueprintable, meta = (DisplayName = "PDGA_ChannelAction"))
class PADO_API UPDGA_ChannelAction : public UPDGA_ActionRuntimeBase
{
	GENERATED_BODY()

public:
	UPDGA_ChannelAction();

	UFUNCTION()
	void HandleExecuteEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageEnded();

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

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual bool TryBeginExecutionWindow() override;

	virtual const FPDLoopingCueStruct* GetLoopingCueConfig() const override;
};
