#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Interface/PDAbilitySourceInterface.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "GameplayAbilitySpec.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDAbilitySystem, Log, All);

bool UPDAbilitySystemComponent::GetDefinitionFromSource(
	const UObject* SourceObject,
	const UPDAbilityDefinition*& OutDefinition,
	FString* OutError)
{
	OutDefinition = nullptr;

	const IPDAbilitySourceInterface* AbilitySource =
		Cast<IPDAbilitySourceInterface>(SourceObject);
	if (!AbilitySource)
	{
		if (OutError)
		{
			*OutError = TEXT("SourceObject가 PDAbilitySourceInterface를 구현하지 않습니다.");
		}
		return false;
	}

	if (!AbilitySource->ResolveAbilityDefinition(OutDefinition) ||
		!IsValid(OutDefinition))
	{
		if (OutError)
		{
			*OutError = TEXT("SourceObject가 Ability Definition을 제공하지 않았습니다.");
		}
		return false;
	}

	return true;
}

bool UPDAbilitySystemComponent::ResolveDefinitionFromSource(
	const UObject* SourceObject,
	const UPDAbilityDefinition*& OutDefinition,
	FString* OutError)
{
	if (!GetDefinitionFromSource(SourceObject, OutDefinition, OutError))
	{
		return false;
	}

	FString ValidationError;
	if (!OutDefinition->ValidateWithActionContract(ValidationError))
	{
		if (OutError)
		{
			*OutError = ValidationError;
		}
		return false;
	}

	return true;
}

FGameplayAbilitySpecHandle UPDAbilitySystemComponent::GrantAbilityFromSource(
	UObject* SourceObject,
	int32 InputId)
{
	// 검증은 GrantAbilityDefinition에서 정확히 한 번 수행한다.
	const UPDAbilityDefinition* Definition = nullptr;
	FString Error;
	if (!GetDefinitionFromSource(SourceObject, Definition, &Error))
	{
		UE_LOG(
			LogPDAbilitySystem,
			Warning,
			TEXT("Ability 부여를 거부했습니다. Source='%s', 이유: %s"),
			*GetNameSafe(SourceObject),
			*Error);
		return FGameplayAbilitySpecHandle();
	}

	return GrantAbilityDefinition(Definition, SourceObject, InputId);
}

FGameplayAbilitySpecHandle UPDAbilitySystemComponent::GrantAbilityDefinition(
	const UPDAbilityDefinition* Definition,
	UObject* SourceObject,
	int32 InputId)
{
	if (!IsOwnerActorAuthoritative() || !IsValid(Definition) ||
		!IsValid(SourceObject))
	{
		return FGameplayAbilitySpecHandle();
	}

	const UPDAbilityDefinition* SourceDefinition = nullptr;
	FString Error;
	if (!ResolveDefinitionFromSource(SourceObject, SourceDefinition, &Error) ||
		Definition != SourceDefinition)
	{
		UE_LOG(
			LogPDAbilitySystem,
			Warning,
			TEXT("Source에서 재구성한 Definition과 요청 Definition이 달라 부여를 거부했습니다. Source='%s', 이유: %s"),
			*GetNameSafe(SourceObject),
			*Error);
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& ExistingSpec : GetActivatableAbilities())
	{
		if (ExistingSpec.SourceObject.Get() != SourceObject)
		{
			continue;
		}

		const bool bSameGrant =
			ExistingSpec.Ability &&
			ExistingSpec.Ability->GetClass() == Definition->GetAbilityClass() &&
			ExistingSpec.Level == Definition->AbilityLevel &&
			ExistingSpec.InputID == InputId;
		if (bSameGrant)
		{
			return ExistingSpec.Handle;
		}

		UE_LOG(
			LogPDAbilitySystem,
			Warning,
			TEXT("같은 SourceObject에 서로 다른 Ability가 이미 부여되어 있습니다. Source='%s'"),
			*GetNameSafe(SourceObject));
		return FGameplayAbilitySpecHandle();
	}

	return GiveAbility(FGameplayAbilitySpec(
		Definition->GetAbilityClass(),
		Definition->AbilityLevel,
		InputId,
		SourceObject));
}

bool UPDAbilitySystemComponent::TryActivateGrantedAbility(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	return AbilityHandle.IsValid() && TryActivateAbility(AbilityHandle);
}

bool UPDAbilitySystemComponent::PressAbilityInputByHandle(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	if (IsOwnerActorAuthoritative())
	{
		return ProcessAbilityInputPressed(AbilityHandle);
	}

	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!Spec || !Spec->Ability)
	{
		return false;
	}

	// 방아쇠는 조종하는 머신만 알면 된다. 서버에 필요한 것은 발 기록에 들어 있다.
	// Block은 발마다 보므로 여기서 막지 않는다. 막혀 있어도 방아쇠 상태는 유지해야
	// 풀리는 순간 이어서 쏜다.
	if (UsesLocalTriggerInput(*Spec))
	{
		if (!Spec->IsActive())
		{
			return false;
		}

		AbilitySpecInputPressed(*Spec);
		return true;
	}

	const FGameplayTagContainer& AbilityTags = Spec->Ability->GetAssetTags();
	if (AbilityTags.HasTag(TAG_PD_Ability_Action) &&
		AreAbilityTagsBlocked(AbilityTags))
	{
		return false;
	}

	// 아직 진행 중인 Action에 다시 들어온 Press는 새로 시작할 것이 없다.
	// Press와 Release 짝이 어긋나지 않게 아무것도 하지 않는다.
	if (Spec->IsActive())
	{
		return false;
	}

	// 소유 클라이언트가 바로 활성화한다. LocalPredicted라 GAS가 예측 키와 함께
	// 서버 활성화를 요청하므로, 서버를 거쳐 되돌아오는 왕복 없이 바로 실행된다.
	AbilitySpecInputPressed(*Spec);
	return TryActivateAbility(AbilityHandle);
}

bool UPDAbilitySystemComponent::ReleaseAbilityInputByHandle(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	if (IsOwnerActorAuthoritative())
	{
		return ProcessAbilityInputReleased(AbilityHandle);
	}

	FGameplayAbilitySpec* LocalTriggerSpec = FindAbilitySpecFromHandle(AbilityHandle);
	if (LocalTriggerSpec && UsesLocalTriggerInput(*LocalTriggerSpec))
	{
		AbilitySpecInputReleased(*LocalTriggerSpec);
		return true;
	}

	// 서버에 먼저 알린다. 아래 로컬 처리로 Action이 끝나면 GAS가 종료를 서버에
	// 따로 알리는데, 같은 채널의 신뢰성 RPC라 이 Release가 먼저 도착한다. 그래서
	// 서버는 Release로 하는 일(Channel 완료, 충전 투척 실행)을 먼저 마친다.
	ServerReleaseAbilityInputByHandle(AbilityHandle);

	// 자기 인스턴스도 바로 Release를 받는다. 서버 종료가 복제되기를 기다리면
	// 그동안 다시 누른 입력이 아직 활성인 Action에 막힌다.
	if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle))
	{
		AbilitySpecInputReleased(*Spec);
	}
	return true;
}

void UPDAbilitySystemComponent::ServerReleaseAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	ProcessAbilityInputReleased(AbilityHandle);
}

bool UPDAbilitySystemComponent::UsesLocalTriggerInput(
	const FGameplayAbilitySpec& Spec)
{
	const UPDGA_Base* Ability = Cast<UPDGA_Base>(Spec.Ability);
	return Ability && Ability->UsesLocalTriggerInput();
}

void UPDAbilitySystemComponent::ServerSubmitFireShots_Implementation(
	const FPDFireShotBatchStruct& Batch)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Batch.AbilityHandle);
	UPDGA_FireAction* FireAction = Spec
		? Cast<UPDGA_FireAction>(Spec->GetPrimaryInstance())
		: nullptr;

	// 무기를 놓아 Spec이 사라진 뒤 도착한 묶음은 버린다. 서버의 보유 상태가 우선이다.
	if (FireAction)
	{
		FireAction->ProcessShotBatch(Batch);
	}
}

void UPDAbilitySystemComponent::MulticastFireShots_Implementation(
	const FPDFireShotBatchStruct& Batch)
{
	// 조종하는 머신은 쏜 순간 이미 보여 줬다.
	if (!AbilityActorInfo.IsValid() || AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	UPDGA_FireAction::PresentShotBatch(this, Batch);
}

bool UPDAbilitySystemComponent::ProcessAbilityInputPressed(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!IsOwnerActorAuthoritative() || !Spec)
	{
		return false;
	}

	AbilitySpecInputPressed(*Spec);
	return Spec->IsActive() || TryActivateAbility(AbilityHandle);
}

bool UPDAbilitySystemComponent::ProcessAbilityInputReleased(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!IsOwnerActorAuthoritative() || !Spec)
	{
		return false;
	}

	AbilitySpecInputReleased(*Spec);
	return true;
}

bool UPDAbilitySystemComponent::TryActivateGrantedAbilityWithEvent(
	FGameplayAbilitySpecHandle AbilityHandle,
	FGameplayTag EventTag,
	const FGameplayEventData& EventData)
{
	if (!AbilityHandle.IsValid() || !EventTag.IsValid() || !AbilityActorInfo.IsValid())
	{
		return false;
	}
	
	return TriggerAbilityFromGameplayEvent(
		AbilityHandle,
		AbilityActorInfo.Get(),
		EventTag,
		&EventData,
		*this);
}

void UPDAbilitySystemComponent::BeginLocalActionCooldown(
	FGameplayTag CooldownTag,
	float Duration)
{
	const UWorld* World = GetWorld();
	if (!World || !CooldownTag.IsValid() || Duration <= 0.0f)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	double StartTime = Now;

	// 끝난 지 한 프레임이 안 됐으면 예정된 시각에 이어서 시작한 것으로 본다.
	// 반복 타이머는 프레임 단위로 늦게 불리고, 프레임이 길면 한 프레임에 여러 번
	// 불린다. 호출된 시각에서 새로 세면 그 오차만큼 간격이 벌어지거나 한 프레임에
	// 몰린 반복이 막혀서, 반복 속도가 프레임 속도에 따라 달라진다.
	if (const double* PreviousEndTime = LocalActionCooldownEndTimes.Find(CooldownTag))
	{
		if (Now >= *PreviousEndTime &&
			Now - *PreviousEndTime <= World->GetDeltaSeconds())
		{
			StartTime = *PreviousEndTime;
		}
	}

	LocalActionCooldownEndTimes.Add(CooldownTag, StartTime + Duration);
}

bool UPDAbilitySystemComponent::IsLocalActionCooldownActive(
	FGameplayTag CooldownTag) const
{
	const UWorld* World = GetWorld();
	const double* EndTime = LocalActionCooldownEndTimes.Find(CooldownTag);

	// 같은 시각끼리의 비교가 부동소수 오차로 막히지 않게 아주 작은 여유를 둔다.
	constexpr double TimeTolerance = 1.0e-4;
	return World && EndTime &&
		World->GetTimeSeconds() + TimeTolerance < *EndTime;
}

float UPDAbilitySystemComponent::GetActionCooldownRemaining(
	FGameplayTag CooldownTag) const
{
	const UWorld* World = GetWorld();
	if (!World || !CooldownTag.IsValid())
	{
		return 0.0f;
	}

	if (AbilityActorInfo.IsValid() && AbilityActorInfo->IsLocallyControlled())
	{
		const double* EndTime = LocalActionCooldownEndTimes.Find(CooldownTag);
		return EndTime
			? FMath::Max(0.0f, static_cast<float>(*EndTime - World->GetTimeSeconds()))
			: 0.0f;
	}

	float Remaining = 0.0f;
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(CooldownTag));
	for (const float EffectRemaining : GetActiveEffectsTimeRemaining(Query))
	{
		Remaining = FMath::Max(Remaining, EffectRemaining);
	}
	return Remaining;
}

bool UPDAbilitySystemComponent::RevokeAbilityByHandle(
	FGameplayAbilitySpecHandle AbilityHandle,
	bool bCancelActiveAbility)
{
	if (!IsOwnerActorAuthoritative() || !AbilityHandle.IsValid() ||
		!FindAbilitySpecFromHandle(AbilityHandle))
	{
		return false;
	}

	if (bCancelActiveAbility)
	{
		CancelAbilityHandle(AbilityHandle);
	}
	ClearAbility(AbilityHandle);
	return true;
}

FGameplayAbilitySpecHandle UPDAbilitySystemComponent::FindGrantedAbilityBySource(
	const UObject* SourceObject) const
{
	if (!IsValid(SourceObject))
	{
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.SourceObject.Get() == SourceObject)
		{
			return Spec.Handle;
		}
	}

	return FGameplayAbilitySpecHandle();
}

void UPDAbilitySystemComponent::ApplyActionMontageHitLagForRemoteOwner(
	UAnimMontage* Montage,
	float EffectivePlayRate,
	float Duration,
	uint32 InHitLagGeneration)
{
	if (!IsValid(Montage) || !FMath::IsFinite(EffectivePlayRate) ||
		EffectivePlayRate <= 0.0f || !FMath::IsFinite(Duration) ||
		Duration <= 0.0f || !IsRemoteOwnerMontageTarget())
	{
		return;
	}

	// 몽타주로 대상을 지목한다. 소유 클라이언트가 같은 몽타주를 이미 예측
	// 재생하고 있으므로 요청 ID로 대조할 필요가 없다.
	ClientApplyActionMontageHitLag(
		Montage,
		EffectivePlayRate,
		Duration,
		InHitLagGeneration);
}

void UPDAbilitySystemComponent::ClientApplyActionMontageHitLag_Implementation(
	UAnimMontage* Montage,
	float EffectivePlayRate,
	float Duration,
	uint32 InHitLagGeneration)
{
	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid()
		? AbilityActorInfo->GetAnimInstance()
		: nullptr;
	UWorld* World = GetWorld();
	if (!IsValid(Montage) || !AnimInstance || !World ||
		!AbilityActorInfo->IsLocallyControlled() ||
		!AnimInstance->Montage_IsPlaying(Montage) ||
		!FMath::IsFinite(EffectivePlayRate) || EffectivePlayRate <= 0.0f ||
		!FMath::IsFinite(Duration) || Duration <= 0.0f)
	{
		return;
	}

	// 늦게 도착한 오래된 역경직이 최신 것을 덮어쓰지 않게 한다.
	if (InHitLagGeneration != 0 && HitLagGeneration != 0 &&
		static_cast<int32>(InHitLagGeneration - HitLagGeneration) <= 0)
	{
		return;
	}

	// 이미 역경직 중이면 그때 저장한 원래 재생률을 유지한다. 낮춘 값을
	// 원본으로 덮어쓰면 복원할 때 느린 상태로 되돌아간다.
	if (HitLagMontage.Get() != Montage || !HitLagTimerHandle.IsValid())
	{
		HitLagRestorePlayRate = AnimInstance->Montage_GetPlayRate(Montage);
	}

	HitLagMontage = Montage;
	HitLagGeneration = InHitLagGeneration;
	AnimInstance->Montage_SetPlayRate(Montage, EffectivePlayRate);
	World->GetTimerManager().SetTimer(
		HitLagTimerHandle,
		this,
		&UPDAbilitySystemComponent::RestoreHitLagPlayRate,
		Duration,
		false);
}

void UPDAbilitySystemComponent::RestoreHitLagPlayRate()
{
	HitLagTimerHandle.Invalidate();
	UAnimMontage* Montage = HitLagMontage.Get();
	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid()
		? AbilityActorInfo->GetAnimInstance()
		: nullptr;
	if (IsValid(Montage) && AnimInstance &&
		AnimInstance->Montage_IsPlaying(Montage))
	{
		AnimInstance->Montage_SetPlayRate(Montage, HitLagRestorePlayRate);
	}

	ClearHitLag();
}

void UPDAbilitySystemComponent::ClearHitLag()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitLagTimerHandle);
	}
	else
	{
		HitLagTimerHandle.Invalidate();
	}

	HitLagMontage.Reset();
	HitLagRestorePlayRate = 1.0f;
	HitLagGeneration = 0;
}

bool UPDAbilitySystemComponent::IsRemoteOwnerMontageTarget() const
{
	// 리슨 서버 호스트는 서버에서 이미 재생하므로 중복 재생을 막는다.
	return IsOwnerActorAuthoritative() && AbilityActorInfo.IsValid() &&
		!AbilityActorInfo->IsLocallyControlled();
}
