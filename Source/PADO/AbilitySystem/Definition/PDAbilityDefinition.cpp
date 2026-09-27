#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"

#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PADO/AbilitySystem/Fragment/PDApplyMontageHitLagFragment.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"

const FPDActionHookStruct* UPDAbilityDefinition::FindActionHook(
	FGameplayTag HookTag) const
{
	return ActionHooks.FindByPredicate(
		[HookTag](const FPDActionHookStruct& Hook)
		{
			return Hook.HookTag.MatchesTagExact(HookTag);
		});
}

bool UPDAbilityDefinition::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return DataTag.IsValid() && ActionHooks.ContainsByPredicate(
		[DataTag](const FPDActionHookStruct& Hook)
		{
			return Hook.DeclaresSetByCallerTag(DataTag);
		});
}

TSubclassOf<UPDGA_Base> UPDAbilityDefinition::GetAbilityClass() const
{
	return nullptr;
}

bool UPDAbilityDefinition::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!GetAbilityClass())
	{
		OutError = TEXT("Definition에 대응하는 Ability 클래스가 없습니다.");
		return false;
	}

	if (AbilityLevel < 1)
	{
		OutError = TEXT("AbilityLevel은 1 이상이어야 합니다.");
		return false;
	}

	FString MontageError;
	if (!ActionMontage.Validate(MontageError))
	{
		OutError = FString::Printf(
			TEXT("ActionMontage가 유효하지 않습니다: %s"),
			*MontageError);
		return false;
	}

	FString CooldownError;
	if (!ActionCooldown.Validate(CooldownError))
	{
		OutError = FString::Printf(
			TEXT("ActionCooldown이 유효하지 않습니다: %s"),
			*CooldownError);
		return false;
	}

	if (!ActionTargeting)
	{
		OutError = TEXT("ActionTargeting이 비어 있습니다. 대상 수집 방식을 지정해야 합니다.");
		return false;
	}

	FString TargetingError;
	if (!ActionTargeting->Validate(TargetingError))
	{
		OutError = FString::Printf(
			TEXT("ActionTargeting이 유효하지 않습니다: %s"),
			*TargetingError);
		return false;
	}

	TSet<FGameplayTag> SeenHooks;
	for (int32 Index = 0; Index < ActionHooks.Num(); ++Index)
	{
		const FPDActionHookStruct& Hook = ActionHooks[Index];
		FString HookError;
		if (!Hook.Validate(HookError))
		{
			OutError = FString::Printf(
				TEXT("ActionHooks[%d]가 유효하지 않습니다: %s"),
				Index,
				*HookError);
			return false;
		}

		if (SeenHooks.Contains(Hook.HookTag))
		{
			OutError = FString::Printf(
				TEXT("Action Hook '%s'가 중복됩니다."),
				*Hook.HookTag.ToString());
			return false;
		}
		SeenHooks.Add(Hook.HookTag);

	}

	return ValidateLifecycle(OutError);
}

bool UPDAbilityDefinition::ValidateWithActionContract(FString& OutError) const
{
	if (!Validate(OutError))
	{
		return false;
	}

	const TSubclassOf<UPDGA_Base> AbilityClass = GetAbilityClass();
	const UPDGA_Base* AbilityCDO = AbilityClass
		? AbilityClass->GetDefaultObject<UPDGA_Base>()
		: nullptr;
	if (!AbilityCDO)
	{
		OutError = TEXT("대응하는 Ability 클래스의 UPDGA_Base CDO를 읽을 수 없습니다.");
		return false;
	}

	for (const FPDActionHookStruct& Hook : ActionHooks)
	{
		if (Hook.HookTag.MatchesTagExact(TAG_PD_ActionHook_OnFirstHit) &&
			!ActionTargeting->ProducesHitResults())
		{
			OutError = TEXT(
				"OnFirstHit Hook은 HitResult를 제공하는 Targeting에서만 사용할 수 있습니다.");
			return false;
		}

		for (const UPDActionFragment* Fragment : Hook.Fragments)
		{
			// 이번 발 결과는 선을 긋는 Targeting이 OnExecuteStart에만 넘긴다.
			// 다른 곳에 두면 조용히 재생되지 않는다.
			if (Fragment && Fragment->RequiresShotResult())
			{
				if (!Hook.HookTag.MatchesTagExact(TAG_PD_ActionHook_OnExecuteStart))
				{
					OutError = TEXT(
						"이번 발 결과를 쓰는 Fragment는 OnExecuteStart Hook에만 배치할 수 있습니다.");
					return false;
				}

				if (!ActionTargeting->ProducesShotResult())
				{
					OutError = TEXT(
						"이번 발 결과를 쓰는 Fragment에는 Aim Line Trace처럼 결과를 만드는 Targeting이 필요합니다.");
					return false;
				}
			}

			if (!Fragment || !Fragment->IsA<UPDApplyMontageHitLagFragment>())
			{
				continue;
			}

			if (!Hook.HookTag.MatchesTagExact(TAG_PD_ActionHook_OnFirstHit))
			{
				OutError = TEXT(
					"Montage Hit Lag Fragment는 OnFirstHit Hook에만 배치할 수 있습니다.");
				return false;
			}

			if (!IsValid(ActionMontage.Montage))
			{
				OutError = TEXT(
					"Montage Hit Lag Fragment를 사용하는 Action에는 Montage가 필요합니다.");
				return false;
			}
		}
	}

	if (!ActionTargeting->IsA<UPDInstantActionTargeting>() &&
		!ActionTargeting->IsA<UPDTraceWindowTargeting>())
	{
		OutError = TEXT("ActionTargeting이 Instant 또는 TraceWindow 수집 계약을 구현하지 않습니다.");
		return false;
	}

	return AbilityCDO->ValidateDefinitionContract(*this, OutError);
}

bool UPDAbilityDefinition::ValidateLifecycle(FString& OutError) const
{
	return true;
}
