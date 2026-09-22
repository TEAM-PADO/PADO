#include "PADO/AbilitySystem/Task/PDAbilityTask_PlayActionMontage.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Struct/PDMontageHitLagConfigStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "TimerManager.h"

UPDAbilityTask_PlayActionMontage* UPDAbilityTask_PlayActionMontage::Create(
	UGameplayAbility* OwningAbility,
	const FPDActionMontageConfigStruct& MontageConfig,
	bool bInListenForExecuteEvent)
{
	UPDAbilityTask_PlayActionMontage* Task =
		NewAbilityTask<UPDAbilityTask_PlayActionMontage>(OwningAbility);
	if (Task)
	{
		Task->AbilityHandle = OwningAbility
			? OwningAbility->GetCurrentAbilitySpecHandle()
			: FGameplayAbilitySpecHandle();
		Task->Montage = MontageConfig.Montage;
		Task->PlayRate = MontageConfig.PlayRate;
		Task->StartSection = MontageConfig.StartSection;
		Task->bListenForExecuteEvent = bInListenForExecuteEvent;
	}
	return Task;
}

void UPDAbilityTask_PlayActionMontage::Activate()
{
	if (!Ability || !IsValid(Montage))
	{
		FinishTask(true);
		return;
	}

	if (bListenForExecuteEvent)
	{
		// 첫 프레임 Notify도 놓치지 않도록 Event 대기를 몽타주보다 먼저 등록한다.
		EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			Ability,
			TAG_PD_GameplayEvent_Action_Execute,
			nullptr,
			false,
			true);
		if (!EventTask)
		{
			FinishTask(true);
			return;
		}
		EventTask->EventReceived.AddDynamic(
			this,
			&UPDAbilityTask_PlayActionMontage::HandleGameplayEvent);
		EventTask->ReadyForActivation();
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		Ability,
		NAME_None,
		Montage,
		PlayRate,
		StartSection,
		true,
		1.0f,
		0.0f,
		true);
	if (!MontageTask)
	{
		FinishTask(true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(
		this,
		&UPDAbilityTask_PlayActionMontage::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(
		this,
		&UPDAbilityTask_PlayActionMontage::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(
		this,
		&UPDAbilityTask_PlayActionMontage::HandleMontageInterrupted);
	MontageTask->ReadyForActivation();

	if (bTerminal)
	{
		return;
	}

	if (UPDAbilitySystemComponent* AbilitySystem =
		Cast<UPDAbilitySystemComponent>(
			Ability->GetAbilitySystemComponentFromActorInfo()))
	{
		AbilitySystem->PlayActionMontageForRemoteOwner(
			AbilityHandle,
			Montage,
			PlayRate,
			StartSection);
	}
}

bool UPDAbilityTask_PlayActionMontage::CanApplyHitLag() const
{
	const UAbilitySystemComponent* AbilitySystem = Ability
		? Ability->GetAbilitySystemComponentFromActorInfo()
		: nullptr;
	return !bTerminal && !bCleaningUp && IsValid(Montage) &&
		AbilitySystem && AbilitySystem->IsOwnerActorAuthoritative() &&
		AbilitySystem->GetCurrentMontage() == Montage;
}

bool UPDAbilityTask_PlayActionMontage::ApplyHitLag(
	const FPDMontageHitLagConfigStruct& HitLag)
{
	FString ValidationError;
	if (!CanApplyHitLag() || !HitLag.Validate(ValidationError))
	{
		return false;
	}

	if (bHitLagActive &&
		HitLag.RetriggerPolicy ==
			EPDMontageHitLagRetriggerPolicy::IgnoreWhileActive)
	{
		return true;
	}

	UAbilitySystemComponent* AbilitySystem =
		Ability->GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!AbilitySystem || !World)
	{
		return false;
	}

	do
	{
		++HitLagGeneration;
	}
	while (HitLagGeneration == 0);

	const float EffectivePlayRate = HitLag.ResolvePlayRate(PlayRate);
	bHitLagActive = true;
	AbilitySystem->CurrentMontageSetPlayRate(EffectivePlayRate);

	if (UPDAbilitySystemComponent* PDAbilitySystem =
		Cast<UPDAbilitySystemComponent>(AbilitySystem))
	{
		PDAbilitySystem->ApplyActionMontageHitLagForRemoteOwner(
			AbilityHandle,
			Montage,
			EffectivePlayRate,
			HitLag.Duration,
			HitLagGeneration);
	}

	World->GetTimerManager().SetTimer(
		HitLagTimerHandle,
		this,
		&UPDAbilityTask_PlayActionMontage::RestoreHitLag,
		HitLag.Duration,
		false);
	return true;
}

void UPDAbilityTask_PlayActionMontage::RestoreHitLag()
{
	if (!bHitLagActive)
	{
		return;
	}

	bHitLagActive = false;
	HitLagTimerHandle.Invalidate();
	UAbilitySystemComponent* AbilitySystem = Ability
		? Ability->GetAbilitySystemComponentFromActorInfo()
		: nullptr;
	if (AbilitySystem && AbilitySystem->IsOwnerActorAuthoritative() &&
		AbilitySystem->GetCurrentMontage() == Montage)
	{
		AbilitySystem->CurrentMontageSetPlayRate(PlayRate);
	}
}

void UPDAbilityTask_PlayActionMontage::OnDestroy(bool bAbilityEnded)
{
	bCleaningUp = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitLagTimerHandle);
	}
	bHitLagActive = false;
	UAbilitySystemComponent* AbilitySystem = Ability
		? Ability->GetAbilitySystemComponentFromActorInfo()
		: nullptr;
	if (!bTerminal && AbilitySystem &&
		AbilitySystem->GetCurrentMontage() == Montage)
	{
		// 부모 태스크가 자식보다 먼저 정리돼도 서버 몽타주가 남지 않게 직접 멈춘다.
		AbilitySystem->CurrentMontageStop();
	}

	// Ability 종료 중에는 GAS가 ActiveTasks를 직접 순회하며 정리한다.
	// 여기서 자식 Task까지 제거하면 같은 배열의 크기가 순회 도중 바뀌어
	// UGameplayAbility::EndAbility의 인덱스가 무효화될 수 있다.
	if (!bAbilityEnded)
	{
		if (EventTask && !EventTask->IsFinished())
		{
			EventTask->EndTask();
		}
		if (MontageTask && !MontageTask->IsFinished())
		{
			MontageTask->EndTask();
		}
	}

	if (UPDAbilitySystemComponent* PDAbilitySystem =
		Cast<UPDAbilitySystemComponent>(AbilitySystem))
	{
		PDAbilitySystem->StopActionMontageForRemoteOwner(
			AbilityHandle,
			Montage);
	}

	EventTask = nullptr;
	MontageTask = nullptr;
	Super::OnDestroy(bAbilityEnded);
}

void UPDAbilityTask_PlayActionMontage::HandleGameplayEvent(
	FGameplayEventData Payload)
{
	if (bTerminal || bCleaningUp || Payload.OptionalObject.Get() != Ability)
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnExecute.Broadcast(Payload);
	}
}

void UPDAbilityTask_PlayActionMontage::HandleMontageCompleted()
{
	FinishTask(false);
}

void UPDAbilityTask_PlayActionMontage::HandleMontageInterrupted()
{
	FinishTask(true);
}

void UPDAbilityTask_PlayActionMontage::FinishTask(bool bInterrupted)
{
	if (bTerminal || bCleaningUp)
	{
		return;
	}

	bTerminal = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bInterrupted)
		{
			OnInterrupted.Broadcast();
		}
		else
		{
			OnCompleted.Broadcast();
		}
	}

	if (!IsFinished())
	{
		EndTask();
	}
}
