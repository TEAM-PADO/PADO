#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayAbilitySpec.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Definition/PDFireActionDefinition.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Struct/PDActionHookStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PADO/AbilitySystem/Task/PDAbilityTask_FireLoop.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDFireAction, Log, All);

namespace PDFireAction
{
	FPDActionExecutionContext MakeTargetContext(
		const FPDActionExecutionContext& BaseContext,
		const FPDFireShotStruct& Shot,
		const FPDFireShotHitStruct& Hit)
	{
		FPDActionExecutionContext Context = BaseContext;
		Context.TargetActor = Hit.Actor.Get();
		Context.TargetAbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.TargetActor);
		if (Hit.bHasHitResult)
		{
			Context.HitResult = Hit.ToHitResult(Shot.TraceStart, Shot.ShotEnd);
			Context.bHasHitResult = true;
		}
		return Context;
	}

	/**
	 * 한 발의 Hook을 순서대로 실행한다. 쏜 머신의 연출, 서버의 결과, 관찰자의
	 * 연출이 모두 이 순서를 지난다. 어떤 Fragment를 돌릴지는 문맥의 실행 범위가
	 * 정한다. OnExecuteStart의 필수 Fragment가 실패하면 대상 Hook을 실행하지 않고
	 * false를 돌려준다.
	 */
	bool ExecuteShotHooks(
		const UPDAbilityDefinition& Definition,
		const FPDActionExecutionContext& BaseContext,
		const FPDFireShotStruct& Shot,
		bool bExecuteTargetHooks)
	{
		if (const FPDActionHookStruct* StartHook =
			Definition.FindActionHook(TAG_PD_ActionHook_OnExecuteStart))
		{
			FPDActionExecutionContext Context = BaseContext;
			Context.TargetAbilitySystem = BaseContext.SourceAbilitySystem;
			Context.TargetActor = BaseContext.SourceActor;
			if (Shot.bHasShotResult)
			{
				Context.ShotResult = Shot.MakeShotResult();
				Context.bHasShotResult = true;
			}

			if (!UPDGA_Base::ExecuteHookFragments(
				TAG_PD_ActionHook_OnExecuteStart,
				*StartHook,
				Context))
			{
				return false;
			}
		}

		if (!bExecuteTargetHooks)
		{
			return true;
		}

		const FPDActionHookStruct* ExecuteHook =
			Definition.FindActionHook(TAG_PD_ActionHook_OnExecute);
		const FPDFireShotHitStruct* FirstHit = nullptr;
		for (const FPDFireShotHitStruct& Hit : Shot.Hits)
		{
			// 그사이 사라진 대상은 그 대상만 건너뛴다.
			if (!IsValid(Hit.Actor.Get()))
			{
				continue;
			}

			if (Hit.bHasHitResult && !FirstHit)
			{
				FirstHit = &Hit;
			}

			if (ExecuteHook)
			{
				UPDGA_Base::ExecuteHookFragments(
					TAG_PD_ActionHook_OnExecute,
					*ExecuteHook,
					MakeTargetContext(BaseContext, Shot, Hit));
			}
		}

		const FPDActionHookStruct* FirstHitHook =
			Definition.FindActionHook(TAG_PD_ActionHook_OnFirstHit);
		if (FirstHitHook && FirstHit)
		{
			UPDGA_Base::ExecuteHookFragments(
				TAG_PD_ActionHook_OnFirstHit,
				*FirstHitHook,
				MakeTargetContext(BaseContext, Shot, *FirstHit));
		}

		return true;
	}
}

UPDGA_FireAction::UPDGA_FireAction()
{
	// 무기 보유를 확정하는 쪽이 서버라서 활성화도 서버가 시작한다. 소유 클라이언트는
	// GAS가 따라 활성화시킨다. 발사는 활성화가 아니므로 이 왕복은 줍는 순간뿐이다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 무기를 드는 순간과 놓는 순간은 발사와 무관하다. 발 단위 Hook만 받는다.
	ResetActionHookContract();
	AddSupportedActionHook(TAG_PD_ActionHook_OnExecuteStart);
	AddSupportedActionHook(TAG_PD_ActionHook_OnExecute);
	AddSupportedActionHook(TAG_PD_ActionHook_OnFirstHit);
}

bool UPDGA_FireAction::UsesLocalTriggerInput() const
{
	return true;
}

void UPDGA_FireAction::OnGiveAbility(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	UAbilitySystemComponent* AbilitySystem = ActorInfo
		? ActorInfo->AbilitySystemComponent.Get()
		: nullptr;
	if (!AbilitySystem || !ActorInfo->IsNetAuthority() || Spec.IsActive())
	{
		return;
	}

	// 무기를 든 순간부터 놓을 때까지 활성이다.
	if (!AbilitySystem->TryActivateAbility(Spec.Handle))
	{
		UE_LOG(
			LogPDFireAction,
			Warning,
			TEXT("Fire Action을 활성화하지 못해 이 무기로 쏠 수 없습니다. Source='%s'"),
			*GetNameSafe(Spec.SourceObject.Get()));
	}
}

bool UPDGA_FireAction::DoesAbilitySatisfyTagRequirements(
	const UAbilitySystemComponent& AbilitySystemComponent,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	return true;
}

bool UPDGA_FireAction::CheckCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	return true;
}

void UPDGA_FireAction::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
}

void UPDGA_FireAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	FireLoop = nullptr;
	ResetShotBatches();
	ShooterHeldItems = nullptr;
	SourceMagazine = nullptr;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive() || IsFinishingAction())
	{
		return;
	}

	// 다른 Ability의 Cancel로 끝나면 다시 활성화할 주체가 없어, 무기를 다시 들기
	// 전까지 쏠 수 없다. 발사를 막아야 하는 상태는 Block 태그로 발마다 본다.
	// 끝나는 경로는 Revoke와 Actor 종료뿐이고, 둘 다 EndAbility를 직접 부른다.
	SetCanBeCanceled(false);

	const UPDFireActionDefinition* Definition =
		Cast<UPDFireActionDefinition>(GetActiveDefinition());
	if (!Definition || !ActorInfo)
	{
		FinishAction(true, false);
		return;
	}

	// 발마다 찾지 않도록 활성화할 때 한 번 찾아 둔다. Grant 중에는 바뀌지 않는다.
	if (const APDWorldItemActor* SourceItem = GetActiveSourceItem())
	{
		SourceMagazine = SourceItem->GetMagazineComponent();
	}
	if (const AActor* Avatar = ActorInfo->AvatarActor.Get())
	{
		ShooterHeldItems = Avatar->FindComponentByClass<UPDHeldItemComponent>();
	}

	// 방아쇠는 조종하는 머신만 해석한다. 서버의 원격 사수 인스턴스는 발 묶음만 받는다.
	if (!ActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (UPDWeaponMagazineComponent* Magazine = SourceMagazine.Get())
	{
		Magazine->BeginLocalShotSession(Handle, LastShotIndex);
	}

	FireLoop = UPDAbilityTask_FireLoop::Create(this, *Definition);
	FireLoop->ReadyForActivation();
}

void UPDGA_FireAction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 한 발의 결과로 끝나는 경우가 있다. 이미 처리한 발은 관찰자에게 보내고 끝낸다.
	FlushShotBatch();
	ResetShotBatches();
	FireLoop = nullptr;

	if (UPDWeaponMagazineComponent* Magazine = SourceMagazine.Get())
	{
		Magazine->EndLocalShotSession(Handle);
	}
	ShooterHeldItems = nullptr;
	SourceMagazine = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UPDGA_FireAction::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	if (FireLoop)
	{
		FireLoop->PressTrigger();
	}
}

void UPDGA_FireAction::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	if (FireLoop)
	{
		FireLoop->ReleaseTrigger();
	}
}

bool UPDGA_FireAction::TryBeginExecutionWindow()
{
	return IsActive() && !IsFinishingAction();
}

bool UPDGA_FireAction::GatesActivationOnPredictedState() const
{
	// 활성화는 무기를 드는 것이다. 탄약은 발마다 본다.
	return false;
}

EPDFireShotOutcome UPDGA_FireAction::TryFireLocalShot()
{
	const EPDFireShotOutcome Gate = EvaluateShotGate();
	if (Gate != EPDFireShotOutcome::Fired)
	{
		return Gate;
	}

	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	const UPDInstantActionTargeting* Targeting =
		Cast<UPDInstantActionTargeting>(Definition->ActionTargeting);
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	if (!Targeting || !AbilitySystem)
	{
		return EPDFireShotOutcome::Exhausted;
	}

	FPDActionTargetingResult Result;
	Targeting->GatherTargets(BuildTargetingContext(), Result);

	FPDFireShotStruct Shot;
	Shot.ShotIndex = ++LastShotIndex;
	if (Result.bHasShotResult)
	{
		Shot.SetShotResult(Result.ShotResult);
	}
	for (const FPDActionTarget& Target : Result.Targets)
	{
		if (IsValid(Target.Actor))
		{
			Shot.Hits.Add(FPDFireShotHitStruct::Make(
				Target.Actor,
				Target.bHasHitResult ? &Target.HitResult : nullptr));
		}
	}

	// 쏜 머신의 화면에는 서버를 기다리지 않고 바로 보여 준다.
	PresentShot(
		AbilitySystem,
		GetCurrentSourceObject(),
		*Definition,
		Shot,
		this);

	APDWorldItemActor* SourceItem = GetActiveSourceItem();
	UPDHeldItemComponent* HeldItems = ShooterHeldItems.Get();
	if (HeldItems && SourceItem)
	{
		HeldItems->NotifyLocalShotFired(SourceItem);
	}

	if (AbilitySystem->IsOwnerActorAuthoritative())
	{
		// 서버가 조종하는 사수(리슨 호스트, AI)는 보낼 곳이 없다. 바로 처리해야
		// 다음 발의 게이트가 이 발의 탄약 소비를 본다.
		if (ProcessShot(Shot, true))
		{
			PendingRelay.Shots.Add(MoveTemp(Shot));
		}
		return EPDFireShotOutcome::Fired;
	}

	// 서버가 처리할 때까지 이 발만큼 탄약을 빼고 보여 주고 판정한다.
	if (UPDWeaponMagazineComponent* Magazine = SourceMagazine.Get())
	{
		Magazine->RecordLocalShot(GetCurrentAbilitySpecHandle(), Shot.ShotIndex);
	}
	PendingShots.Shots.Add(MoveTemp(Shot));
	return EPDFireShotOutcome::Fired;
}

void UPDGA_FireAction::FlushShotBatch()
{
	UPDAbilitySystemComponent* AbilitySystem =
		Cast<UPDAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (!AbilitySystem)
	{
		ResetShotBatches();
		return;
	}

	if (!PendingShots.Shots.IsEmpty())
	{
		PendingShots.AbilityHandle = GetCurrentAbilitySpecHandle();
		PendingShots.SourceObject = GetCurrentSourceObject();
		AbilitySystem->ServerSubmitFireShots(PendingShots);
		PendingShots.Shots.Reset();
	}

	if (!PendingRelay.Shots.IsEmpty())
	{
		PendingRelay.AbilityHandle = GetCurrentAbilitySpecHandle();
		PendingRelay.SourceObject = GetCurrentSourceObject();
		AbilitySystem->MulticastFireShots(PendingRelay);
		PendingRelay.Shots.Reset();
	}
}

void UPDGA_FireAction::ProcessShotBatch(const FPDFireShotBatchStruct& Batch)
{
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();

	// 무기를 놓은 뒤 도착한 묶음은 버린다. 서버가 확정한 보유 상태가 우선이다.
	if (!AbilitySystem || !AbilitySystem->IsOwnerActorAuthoritative() ||
		!IsActive() || IsFinishingAction() ||
		Batch.AbilityHandle != GetCurrentAbilitySpecHandle() ||
		Batch.SourceObject.Get() != GetCurrentSourceObject())
	{
		return;
	}

	// 판단하려는 것은 순간이 아니라 환경이다. 서버가 잰 평균 핑을 쓴다.
	const bool bAcceptHits =
		ResolveShooterPingMilliseconds() <= MaxAcceptedPingMilliseconds;
	UPDWeaponMagazineComponent* Magazine = SourceMagazine.Get();
	for (const FPDFireShotStruct& Shot : Batch.Shots)
	{
		// 앞 발의 결과로 무기를 놓았으면 남은 발은 쏠 수 없었던 것이다.
		if (!IsActive() || IsFinishingAction())
		{
			break;
		}

		if (ProcessShot(Shot, bAcceptHits))
		{
			PendingRelay.Shots.Add(Shot);
		}

		// 무효가 된 발도 처리한 발로 기록한다. 탄약은 줄지 않았으므로 소유
		// 클라이언트의 예측이 서버 값으로 수렴한다.
		if (Magazine)
		{
			Magazine->RecordProcessedShot(GetCurrentAbilitySpecHandle(), Shot.ShotIndex);
		}
	}

	FlushShotBatch();
}

void UPDGA_FireAction::PresentShotBatch(
	UAbilitySystemComponent* ShooterAbilitySystem,
	const FPDFireShotBatchStruct& Batch)
{
	UObject* SourceObject = Batch.SourceObject.Get();
	const UPDAbilityDefinition* Definition = nullptr;
	if (!ShooterAbilitySystem || !IsValid(SourceObject) ||
		!UPDAbilitySystemComponent::GetDefinitionFromSource(SourceObject, Definition) ||
		!Definition->IsA<UPDFireActionDefinition>())
	{
		// 무기나 Definition이 아직 복제되지 않았다. 연출만 빠지고 결과는 같다.
		return;
	}

	for (const FPDFireShotStruct& Shot : Batch.Shots)
	{
		PresentShot(ShooterAbilitySystem, SourceObject, *Definition, Shot, nullptr);
	}
}

EPDFireShotOutcome UPDGA_FireAction::EvaluateShotGate() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	if (!IsActive() || IsFinishingAction() || !ActorInfo || !AbilitySystem ||
		!Definition || !IsValid(GetCurrentSourceObject()))
	{
		return EPDFireShotOutcome::Exhausted;
	}

	// 무기를 놓았거나 바꿨으면 이 활성화로는 더 쏠 수 없다.
	if (const APDWorldItemActor* SourceItem = GetActiveSourceItem())
	{
		if (!SourceItem->IsUsable() ||
			SourceItem->GetItemState() != EPDWorldItemState::Held ||
			SourceItem->GetHolder() != ActorInfo->AvatarActor.Get())
		{
			return EPDFireShotOutcome::Exhausted;
		}
	}

	// 막힌 동안은 쏘지 않되 방아쇠 상태는 유지한다.
	if (AbilitySystem->AreAbilityTagsBlocked(GetAssetTags()))
	{
		return EPDFireShotOutcome::Blocked;
	}

	// 탄약처럼 실행 비용이 남았는지다. 소유 클라이언트는 예측 탄약을 본다.
	if (!PassesPredictedStateGate(
		*Definition,
		GetCurrentAbilitySpecHandle(),
		ActorInfo))
	{
		return EPDFireShotOutcome::Exhausted;
	}

	return EPDFireShotOutcome::Fired;
}

bool UPDGA_FireAction::ProcessShot(
	const FPDFireShotStruct& Shot,
	bool bAcceptHits)
{
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	const UPDAbilityDefinition* Definition = GetActiveDefinition();
	if (!AbilitySystem || !AbilitySystem->IsOwnerActorAuthoritative() ||
		!Definition || !IsActive() || IsFinishingAction())
	{
		return false;
	}

	// 서버만 아는 무력화(사망, 기절)는 서버가 우선이다. 예측 Ability의 활성과
	// 종료는 발 묶음과 같은 채널의 신뢰성 RPC라 쏜 머신이 본 순서대로 도착하므로,
	// 여기서 어긋나는 것은 서버만 아는 상태뿐이다.
	if (AbilitySystem->AreAbilityTagsBlocked(GetAssetTags()))
	{
		return false;
	}

	FPDActionExecutionContext BaseContext =
		MakeHookContext(AbilitySystem, GetAvatarActorFromActorInfo(), nullptr, nullptr);
	BaseContext.ExecutionScope = EPDActionExecutionScope::ResultsOnly;
	BaseContext.ShotIndex = Shot.ShotIndex;

	// 받은 판정을 다시 추적하지 않는다. 지연 한계를 넘으면 명중만 인정하지 않는다.
	// 쏜 것 자체는 사실이므로 탄약 소비는 그대로 한다.
	return PDFireAction::ExecuteShotHooks(
		*Definition,
		BaseContext,
		Shot,
		bAcceptHits);
}

float UPDGA_FireAction::ResolveShooterPingMilliseconds() const
{
	const APawn* ShooterPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const APlayerState* PlayerState = ShooterPawn
		? ShooterPawn->GetPlayerState()
		: nullptr;

	// PlayerState가 없는 사수(AI)와 서버 자신의 플레이어는 지연이 없다.
	return PlayerState ? PlayerState->GetPingInMilliseconds() : 0.0f;
}

void UPDGA_FireAction::PresentShot(
	UAbilitySystemComponent* ShooterAbilitySystem,
	UObject* SourceObject,
	const UPDAbilityDefinition& Definition,
	const FPDFireShotStruct& Shot,
	UPDGA_Base* Ability)
{
	const UWorld* World = ShooterAbilitySystem
		? ShooterAbilitySystem->GetWorld()
		: nullptr;

	// 데디케이티드 서버에는 보는 사람이 없다.
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FPDActionExecutionContext BaseContext;
	BaseContext.Ability = Ability;
	BaseContext.SourceAbilitySystem = ShooterAbilitySystem;
	BaseContext.SourceActor = ShooterAbilitySystem->GetAvatarActor();
	BaseContext.EffectSourceObject = SourceObject;
	BaseContext.ExecutionScope = EPDActionExecutionScope::PresentationOnly;
	BaseContext.ShotIndex = Shot.ShotIndex;

	PDFireAction::ExecuteShotHooks(Definition, BaseContext, Shot, true);
}

void UPDGA_FireAction::ResetShotBatches()
{
	PendingShots = FPDFireShotBatchStruct();
	PendingRelay = FPDFireShotBatchStruct();
}
