#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
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

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const FGameplayTagContainer* AbilityTags = Spec && Spec->Ability
		? &Spec->Ability->GetAssetTags()
		: nullptr;
	if (AbilityTags && AbilityTags->HasTag(TAG_PD_Ability_Action) &&
		AreAbilityTagsBlocked(*AbilityTags))
	{
		return false;
	}

	// 몽타주 선재생과 요청 ID 대조는 더 이상 없다. LocalPredicted 어빌리티가
	// 소유 클라이언트에서 직접 활성화되어 몽타주와 Cue를 예측 재생하고,
	// 중복 활성화 거부와 예측 회수는 GAS 예측 키가 처리한다.
	ServerPressAbilityInputByHandle(AbilityHandle);
	return true;
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

	ServerReleaseAbilityInputByHandle(AbilityHandle);
	return true;
}

void UPDAbilitySystemComponent::ServerPressAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	ProcessAbilityInputPressed(AbilityHandle);
}

void UPDAbilitySystemComponent::ServerReleaseAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	ProcessAbilityInputReleased(AbilityHandle);
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
