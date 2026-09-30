#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Struct/PDLoopingCueStruct.h"
#include "PADO/AbilitySystem/Struct/PDMontageHitLagConfigStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PADO/AbilitySystem/Task/PDAbilityTask_ActionTraceWindow.h"
#include "PADO/AbilitySystem/Task/PDAbilityTask_PlayActionMontage.h"
#include "PADO/Item/PDWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDActionRuntime, Log, All);

UPDGA_ActionRuntimeBase::UPDGA_ActionRuntimeBase()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_PD_Ability_Action);
	SetAssetTags(AssetTags);

	AddSupportedActionHook(TAG_PD_ActionHook_OnStart);
	AddSupportedActionHook(TAG_PD_ActionHook_OnExecuteStart);
	AddSupportedActionHook(TAG_PD_ActionHook_OnExecute);
	AddSupportedActionHook(TAG_PD_ActionHook_OnFirstHit);
	AddSupportedActionHook(TAG_PD_ActionHook_OnComplete);
}

bool UPDGA_ActionRuntimeBase::CanApplyMontageHitLag() const
{
	return IsValid(ActiveMontageTask) && ActiveMontageTask->CanApplyHitLag();
}

bool UPDGA_ActionRuntimeBase::ApplyMontageHitLag(
	const FPDMontageHitLagConfigStruct& HitLag)
{
	return CanApplyMontageHitLag() && ActiveMontageTask->ApplyHitLag(HitLag);
}

bool UPDGA_ActionRuntimeBase::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	const APDWorldItemActor* SourceItem = ResolveSourceItem(Handle, ActorInfo);
	return !SourceItem || SourceItem->IsUsable();
}

void UPDGA_ActionRuntimeBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ActivationTarget = nullptr;
	ActivationHitResult = FHitResult();
	bHasActivationHitResult = false;
	bFinishingAction = false;
	bActionExecutionStarted = false;
	bFirstHitHookExecuted = false;
	StopLoopingCue();
	ActiveSourceItem = nullptr;
	ActiveMontageTask = nullptr;
	ActiveTraceTask = nullptr;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}
	ActiveSourceItem = ResolveSourceItem(Handle, ActorInfo);

	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	const UPDActionTargeting* Targeting =
		Definition ? Definition->ActionTargeting : nullptr;
	if (!Targeting)
	{
		UE_LOG(
			LogPDActionRuntime,
			Warning,
			TEXT("Definition에 ActionTargeting이 없어 대상을 정할 수 없습니다."));
		FinishAction(true, false);
		return;
	}

	if (TriggerEventData)
	{
		ActivationTarget = const_cast<AActor*>(TriggerEventData->Target.Get());
		if (const FHitResult* Hit = TriggerEventData->ContextHandle.GetHitResult())
		{
			ActivationHitResult = *Hit;
			bHasActivationHitResult = true;
		}
	}

	if (Targeting->RequiresActivationTarget() && !ActivationTarget.IsValid())
	{
		UE_LOG(
			LogPDActionRuntime,
			Warning,
			TEXT("'%s'는 활성화 이벤트의 Target이 필요합니다. ")
			TEXT("소스가 TryActivateWithTarget()으로 발동해야 합니다."),
			*Targeting->GetClass()->GetName());
		FinishAction(true, false);
		return;
	}

	const UPDInstantActionTargeting* InstantTargeting =
		Cast<UPDInstantActionTargeting>(Targeting);
	const UPDTraceWindowTargeting* TraceWindowTargeting =
		Cast<UPDTraceWindowTargeting>(Targeting);
	if (!InstantTargeting && !TraceWindowTargeting)
	{
		UE_LOG(
			LogPDActionRuntime,
			Warning,
			TEXT("Targeting '%s'가 Instant 또는 TraceWindow 계약을 구현하지 않습니다."),
			*Targeting->GetClass()->GetName());
		FinishAction(true, false);
		return;
	}

	if (TraceWindowTargeting)
	{
		UObject* TraceSource = nullptr;
		FString TraceSourceError;
		if (!TraceWindowTargeting->ResolveTraceSource(
			BuildTargetingContext(), TraceSource, &TraceSourceError) ||
			!IsValid(TraceSource))
		{
			if (TraceSourceError.IsEmpty())
			{
				TraceSourceError = TEXT("유효한 런타임 Trace Source를 반환하지 않았습니다.");
			}
			UE_LOG(
				LogPDActionRuntime,
				Warning,
				TEXT("TraceWindow Targeting을 시작할 수 없습니다: %s"),
				*TraceSourceError);
			FinishAction(true, false);
			return;
		}
	}

	if (!ShouldDeferActionExecutionStart())
	{
		BeginActionExecution();
	}
}

void UPDGA_ActionRuntimeBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bFinishingAction = true;
	// 정상 종료·취소·몽타주 중단이 모두 여기를 지난다. Cue 정리를 Hook에 두면 샌다.
	StopLoopingCue();
	ActivationTarget = nullptr;
	ActivationHitResult = FHitResult();
	bHasActivationHitResult = false;
	bActionExecutionStarted = false;
	bFirstHitHookExecuted = false;
	// Ability 종료 중에는 GAS가 Task 배열을 순회해 직접 정리한다.
	ActiveMontageTask = nullptr;
	ActiveTraceTask = nullptr;
	ActiveSourceItem = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

bool UPDGA_ActionRuntimeBase::BeginExecutionWindow()
{
	return TryBeginExecutionWindow() && RunExecuteStartHook(nullptr);
}

bool UPDGA_ActionRuntimeBase::RunExecuteStartHook(const FHitResult* ShotResult)
{
	bFirstHitHookExecuted = false;

	// 대상에게 결과를 주기 전에 실행한다. 명중 여부와 무관한 연출이 여기에 온다.
	if (!ExecuteSourceHook(TAG_PD_ActionHook_OnExecuteStart, ShotResult))
	{
		// 발사 비용처럼 필수인 Fragment가 실패하면 이 실행 구간 전체를 중단한다.
		FinishAction(true, false);
		return false;
	}

	// Hook이 Ability를 끝냈다면 이어서 대상에게 결과를 주지 않는다.
	return !IsFinishingAction();
}

void UPDGA_ActionRuntimeBase::ExecutePulse()
{
	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	const UPDInstantActionTargeting* Targeting = Cast<UPDInstantActionTargeting>(
		Definition ? Definition->ActionTargeting : nullptr);
	if (!bActionExecutionStarted || !Targeting || !TryBeginExecutionWindow())
	{
		return;
	}

	// 대상을 먼저 모아 OnExecuteStart가 이번 발이 멈춘 곳을 받게 한다.
	// 판정은 상태를 바꾸지 않으므로 탄약 같은 비용보다 먼저 해도 된다.
	// 비용이 실패하면 모은 대상은 쓰지 않고 버린다.
	FPDActionTargetingResult Result;
	Targeting->GatherTargets(BuildTargetingContext(), Result);
	if (!RunExecuteStartHook(
		Result.bHasShotResult ? &Result.ShotResult : nullptr))
	{
		return;
	}

	ExecuteTargets(Result.Targets);
}

bool UPDGA_ActionRuntimeBase::ShouldDeferActionExecutionStart() const
{
	return false;
}

bool UPDGA_ActionRuntimeBase::BeginActionExecution()
{
	if (bActionExecutionStarted)
	{
		return true;
	}

	if (IsFinishingAction() || !IsActive())
	{
		return false;
	}

	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo =
		GetCurrentActivationInfo();
	if (!ActorInfo ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAction(true, false);
		return false;
	}

	bActionExecutionStarted = true;

	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	const UPDTraceWindowTargeting* TraceWindowTargeting =
		Cast<UPDTraceWindowTargeting>(
			Definition ? Definition->ActionTargeting : nullptr);
	if (TraceWindowTargeting)
	{
		ActiveTraceTask = UPDAbilityTask_ActionTraceWindow::Create(
			this,
			TraceWindowTargeting);
		if (!ActiveTraceTask)
		{
			FinishAction(true, false);
			return false;
		}
		ActiveTraceTask->ReadyForActivation();
	}

	// OnStart Hook이 같은 태그로 일회성 Cue를 쏠 수 있으므로 먼저 붙여 둔다.
	StartLoopingCue();
	ExecuteSourceHook(TAG_PD_ActionHook_OnStart);
	return true;
}

bool UPDGA_ActionRuntimeBase::HasActionExecutionStarted() const
{
	return bActionExecutionStarted;
}

APDWorldItemActor* UPDGA_ActionRuntimeBase::GetActiveSourceItem() const
{
	return ActiveSourceItem.Get();
}

FPDActionTargetingContext UPDGA_ActionRuntimeBase::BuildTargetingContext() const
{
	FPDActionTargetingContext Context;
	Context.SourceActor = GetAvatarActorFromActorInfo();
	Context.SourceObject = GetCurrentSourceObject();
	Context.ActivationTarget = ActivationTarget.Get();
	Context.ActivationHitResult = ActivationHitResult;
	Context.bHasActivationHitResult = bHasActivationHitResult;
	return Context;
}

bool UPDGA_ActionRuntimeBase::ExecuteTargets(
	const TArray<FPDActionTarget>& Targets)
{
	bool bAnyExecutionSucceeded = false;
	int32 FirstHitIndex = INDEX_NONE;
	for (int32 TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
	{
		const FPDActionTarget& Target = Targets[TargetIndex];
		if (!IsValid(Target.Actor))
		{
			continue;
		}
		if (Target.bHasHitResult)
		{
			if (FirstHitIndex == INDEX_NONE)
			{
				FirstHitIndex = TargetIndex;
			}
		}

		UAbilitySystemComponent* TargetAbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target.Actor);
		bAnyExecutionSucceeded |= ExecuteActionHook(
			TAG_PD_ActionHook_OnExecute,
			TargetAbilitySystem,
			Target.Actor,
			Target.bHasHitResult ? &Target.HitResult : nullptr);
	}

	if (FirstHitIndex != INDEX_NONE && !bFirstHitHookExecuted)
	{
		bFirstHitHookExecuted = true;
		const FPDActionTarget& FirstHitTarget = Targets[FirstHitIndex];
		UAbilitySystemComponent* TargetAbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
				FirstHitTarget.Actor);
		bAnyExecutionSucceeded |= ExecuteActionHook(
			TAG_PD_ActionHook_OnFirstHit,
			TargetAbilitySystem,
			FirstHitTarget.Actor,
			&FirstHitTarget.HitResult);
	}
	return bAnyExecutionSucceeded;
}

APDWorldItemActor* UPDGA_ActionRuntimeBase::ResolveSourceItem(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec =
		ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	const UPDAbilitySourceComponent* SourceComponent = Spec
		? Cast<UPDAbilitySourceComponent>(Spec->SourceObject.Get())
		: nullptr;
	return SourceComponent
		? Cast<APDWorldItemActor>(SourceComponent->GetOwner())
		: nullptr;
}

void UPDGA_ActionRuntimeBase::NotifySocketTraceWindowBegin()
{
	if (ActiveTraceTask)
	{
		ActiveTraceTask->BeginTraceWindow();
	}
}

void UPDGA_ActionRuntimeBase::NotifySocketTraceWindowTick()
{
	if (ActiveTraceTask)
	{
		ActiveTraceTask->TickTraceWindow();
	}
}

void UPDGA_ActionRuntimeBase::NotifySocketTraceWindowEnd()
{
	if (ActiveTraceTask)
	{
		ActiveTraceTask->EndTraceWindow();
	}
}

const FPDLoopingCueStruct* UPDGA_ActionRuntimeBase::GetLoopingCueConfig() const
{
	return nullptr;
}

void UPDGA_ActionRuntimeBase::StartLoopingCue()
{
	const FPDLoopingCueStruct* LoopingCue = GetLoopingCueConfig();
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	if (!LoopingCue || !LoopingCue->IsEnabled() || !AbilitySystem ||
		!AbilitySystem->IsOwnerActorAuthoritative() ||
		ActiveLoopingCueTag.IsValid())
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APDWorldItemActor* SourceItem = ActiveSourceItem.Get();
	UObject* SourceObject = GetCurrentSourceObject();
	AActor* EffectCauser = IsValid(SourceItem) ? SourceItem : AvatarActor;

	// Instigator는 논리적 주체(ASC 소유자), EffectCauser는 물리적 원인(아이템)이다.
	// Cue 파라미터의 Instigator는 연출 기준이라 아바타로 둔다.
	FGameplayEffectContextHandle EffectContext =
		AbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(AbilitySystem->GetOwnerActor(), EffectCauser);
	if (SourceObject)
	{
		EffectContext.AddSourceObject(SourceObject);
	}

	FGameplayCueParameters CueParameters(EffectContext);
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = EffectCauser;
	CueParameters.SourceObject = SourceObject;
	CueParameters.AbilityLevel = GetAbilityLevel();

	/**
	 * Location을 비워 둬야 Cue Notify가 부착 대상의 소켓 Transform으로 배치된다.
	 * 위치와 방향이 모두 아이템을 따라가므로 방향을 따로 계산할 필요가 없다.
	 */
	if (LoopingCue->bAttachToSourceItem && IsValid(SourceItem))
	{
		CueParameters.TargetAttachComponent = SourceItem->GetItemMesh();
	}

	AbilitySystem->AddGameplayCue(LoopingCue->CueTag, CueParameters);
	ActiveLoopingCueTag = LoopingCue->CueTag;
}

void UPDGA_ActionRuntimeBase::StopLoopingCue()
{
	if (!ActiveLoopingCueTag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo())
	{
		AbilitySystem->RemoveGameplayCue(ActiveLoopingCueTag);
	}
	ActiveLoopingCueTag = FGameplayTag();
}

bool UPDGA_ActionRuntimeBase::ExecuteSourceHook(
	FGameplayTag HookTag,
	const FHitResult* ShotResult)
{
	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition)
	{
		return false;
	}

	if (!Definition->FindActionHook(HookTag))
	{
		return true;
	}

	return ExecuteActionHook(
		HookTag,
		GetAbilitySystemComponentFromActorInfo(),
		GetAvatarActorFromActorInfo(),
		nullptr,
		ShotResult);
}

void UPDGA_ActionRuntimeBase::FinishAction(
	bool bWasCancelled,
	bool bRunCompleteHook)
{
	if (bFinishingAction || !IsActive())
	{
		return;
	}

	bFinishingAction = true;
	if (!bWasCancelled && bRunCompleteHook)
	{
		ExecuteSourceHook(TAG_PD_ActionHook_OnComplete);
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}

bool UPDGA_ActionRuntimeBase::IsFinishingAction() const
{
	return bFinishingAction;
}
