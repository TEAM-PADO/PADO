#include "PADO/AbilitySystem/Ability/PDGA_Action.h"

#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/AbilitySystem/Fragment/PDThrowProjectileFragment.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PADO/AbilitySystem/Task/PDAbilityTask_PlayActionMontage.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDSingleAction, Log, All);

UPDGA_Action::UPDGA_Action()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPDGA_Action::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bExecuteAttempted = false;
	bWaitingForInputRelease = false;
	ChargeStartTimeSeconds = 0.0;
	ActiveMontageTask = nullptr;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	if (ShouldDeferActionExecutionStart())
	{
		bWaitingForInputRelease = true;
		ChargeStartTimeSeconds = GetWorld()
			? GetWorld()->GetTimeSeconds()
			: 0.0;
		return;
	}

	BeginExecutionSequence();
}

void UPDGA_Action::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	if (!bWaitingForInputRelease || IsFinishingAction() || !IsActive())
	{
		return;
	}

	bWaitingForInputRelease = false;
	const UPDSingleActionDefinition* Definition =
		Cast<UPDSingleActionDefinition>(GetActiveDefinition());
	const UPDThrowProjectileFragment* ChargedThrow = Definition
		? Definition->FindChargedThrowProjectileFragment()
		: nullptr;
	const double CurrentTimeSeconds = GetWorld()
		? GetWorld()->GetTimeSeconds()
		: ChargeStartTimeSeconds;
	const float HeldDuration = static_cast<float>(FMath::Max(
		0.0,
		CurrentTimeSeconds - ChargeStartTimeSeconds));
	SetExecutionChargeAlpha(ChargedThrow
		? ChargedThrow->LaunchConfig.CalculateChargeAlpha(HeldDuration)
		: 1.0f);

	if (!BeginActionExecution())
	{
		return;
	}

	BeginExecutionSequence();
}

void UPDGA_Action::BeginExecutionSequence()
{
	if (!HasActionExecutionStarted() || IsFinishingAction() || !IsActive())
	{
		return;
	}

	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition)
	{
		FinishAction(true, false);
		return;
	}

	if (!IsValid(Definition->ActionMontage.Montage))
	{
		ExecutePulse();
		FinishAction(!bExecuteAttempted, bExecuteAttempted);
		return;
	}

	ActiveMontageTask = UPDAbilityTask_PlayActionMontage::Create(
		this,
		Definition->ActionMontage,
		Definition->ActionTargeting->IsA<UPDInstantActionTargeting>());
	if (!ActiveMontageTask)
	{
		FinishAction(true, false);
		return;
	}

	ActiveMontageTask->OnExecute.AddDynamic(
		this,
		&UPDGA_Action::HandleExecuteEvent);
	ActiveMontageTask->OnCompleted.AddDynamic(
		this,
		&UPDGA_Action::HandleMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(
		this,
		&UPDGA_Action::HandleMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

bool UPDGA_Action::ShouldDeferActionExecutionStart() const
{
	const UPDSingleActionDefinition* Definition =
		Cast<UPDSingleActionDefinition>(GetActiveDefinition());
	return Definition && Definition->ExecutesOnInputRelease();
}

void UPDGA_Action::HandleExecuteEvent(FGameplayEventData Payload)
{
	ExecutePulse();
}

void UPDGA_Action::HandleMontageCompleted()
{
	if (IsFinishingAction())
	{
		return;
	}

	if (!bExecuteAttempted)
	{
		const UPDAbilityDefinition* Definition = GetActiveDefinition();
		const bool bTraceWindowExpected = Definition &&
			Definition->ActionTargeting &&
			Definition->ActionTargeting->IsA<UPDTraceWindowTargeting>();
		UE_LOG(
			LogPDSingleAction,
			Warning,
			TEXT("몽타주가 끝날 때까지 %s가 실행되지 않아 Action을 취소했습니다."),
			bTraceWindowExpected
				? TEXT("PD Action Socket Trace Window")
				: *TAG_PD_GameplayEvent_Action_Execute.GetTag().ToString());
		FinishAction(true, false);
		return;
	}

	FinishAction(false, true);
}

void UPDGA_Action::HandleMontageInterrupted()
{
	FinishAction(true, false);
}

bool UPDGA_Action::TryBeginExecutionWindow()
{
	if (IsFinishingAction() || bExecuteAttempted)
	{
		return false;
	}

	bExecuteAttempted = true;
	return true;
}
