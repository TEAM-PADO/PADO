#include "PADO/AbilitySystem/Ability/PDGA_ChannelAction.h"

#include "Engine/World.h"
#include "PADO/AbilitySystem/Definition/PDChannelActionDefinition.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PADO/AbilitySystem/Task/PDAbilityTask_PlayActionMontage.h"
#include "TimerManager.h"

UPDGA_ChannelAction::UPDGA_ChannelAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UPDGA_ChannelAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ActiveMontageTask = nullptr;
	StopFixedIntervalExecution();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	const UPDChannelActionDefinition* Definition =
		Cast<UPDChannelActionDefinition>(GetActiveDefinition());
	if (!Definition)
	{
		FinishAction(true, false);
		return;
	}

	// MontageEvent는 몽타주 Notify가 유일한 실행 시점이라 몽타주 없이는 성립하지 않는다.
	const bool bHasMontage = IsValid(Definition->ActionMontage.Montage);
	if (!bHasMontage &&
		Definition->ExecutionMode == EPDChannelExecutionMode::MontageEvent)
	{
		FinishAction(true, false);
		return;
	}

	if (bHasMontage)
	{
		const bool bListenForExecuteEvent =
			Definition->ExecutionMode == EPDChannelExecutionMode::MontageEvent &&
			Definition->ActionTargeting->IsA<UPDInstantActionTargeting>();
		ActiveMontageTask = UPDAbilityTask_PlayActionMontage::Create(
			this,
			Definition->ActionMontage,
			bListenForExecuteEvent);
		if (!ActiveMontageTask)
		{
			FinishAction(true, false);
			return;
		}

		if (bListenForExecuteEvent)
		{
			ActiveMontageTask->OnExecute.AddDynamic(
				this,
				&UPDGA_ChannelAction::HandleExecuteEvent);
		}
		// 몽타주가 끝나면 채널도 끝난다. 유지형 연출에는 루프 몽타주를 써야 한다.
		ActiveMontageTask->OnCompleted.AddDynamic(
			this,
			&UPDGA_ChannelAction::HandleMontageEnded);
		ActiveMontageTask->OnInterrupted.AddDynamic(
			this,
			&UPDGA_ChannelAction::HandleMontageEnded);
		ActiveMontageTask->ReadyForActivation();
	}

	if (IsActive() && !IsFinishingAction() &&
		Definition->ExecutionMode == EPDChannelExecutionMode::FixedInterval)
	{
		StartFixedIntervalExecution();
	}
}

void UPDGA_ChannelAction::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	FinishAction(false, true);
}

void UPDGA_ChannelAction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopFixedIntervalExecution();
	ActiveMontageTask = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UPDGA_ChannelAction::HandleExecuteEvent(FGameplayEventData Payload)
{
	if (!IsFinishingAction())
	{
		ExecutePulse();
	}
}

void UPDGA_ChannelAction::HandleMontageEnded()
{
	// Channel은 Release가 정상 종료다. 재생 완료·중단은 모두 비정상 종료다.
	FinishAction(true, false);
}

bool UPDGA_ChannelAction::TryBeginExecutionWindow()
{
	return !IsFinishingAction();
}

const FPDLoopingCueStruct* UPDGA_ChannelAction::GetLoopingCueConfig() const
{
	const UPDChannelActionDefinition* Definition =
		Cast<UPDChannelActionDefinition>(GetActiveDefinition());
	return Definition ? &Definition->LoopingCue : nullptr;
}

void UPDGA_ChannelAction::StartFixedIntervalExecution()
{
	const UPDChannelActionDefinition* Definition =
		Cast<UPDChannelActionDefinition>(GetActiveDefinition());
	UWorld* World = GetWorld();
	if (!Definition || !World ||
		Definition->ExecutionMode != EPDChannelExecutionMode::FixedInterval)
	{
		FinishAction(true, false);
		return;
	}

	World->GetTimerManager().SetTimer(
		FixedIntervalTimerHandle,
		this,
		&UPDGA_ChannelAction::HandleFixedIntervalPulse,
		Definition->PulseInterval,
		true);

	if (Definition->bExecuteImmediately && IsActive() && !IsFinishingAction())
	{
		ExecutePulse();
	}
}

void UPDGA_ChannelAction::StopFixedIntervalExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FixedIntervalTimerHandle);
	}
	else
	{
		FixedIntervalTimerHandle.Invalidate();
	}
}

void UPDGA_ChannelAction::HandleFixedIntervalPulse()
{
	if (!IsActive() || IsFinishingAction())
	{
		StopFixedIntervalExecution();
		return;
	}

	ExecutePulse();
}
