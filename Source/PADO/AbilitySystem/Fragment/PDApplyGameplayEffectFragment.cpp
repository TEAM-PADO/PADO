#include "PADO/AbilitySystem/Fragment/PDApplyGameplayEffectFragment.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Struct/PDSetByCallerValueStruct.h"

bool UPDApplyGameplayEffectFragment::Validate(FString& OutError) const
{
	if (!EffectRecipe.Validate(OutError))
	{
		return false;
	}

	if (bTrackUntilAbilityEnds)
	{
		const UGameplayEffect* EffectCDO = EffectRecipe.EffectClass
			? EffectRecipe.EffectClass->GetDefaultObject<UGameplayEffect>()
			: nullptr;
		if (!EffectCDO ||
			EffectCDO->DurationPolicy != EGameplayEffectDurationType::Infinite)
		{
			OutError = FString::Printf(
				TEXT("GA 종료까지 추적할 Effect '%s'는 Infinite Duration이어야 합니다."),
				*GetNameSafe(EffectCDO));
			return false;
		}
	}

	return true;
}

bool UPDApplyGameplayEffectFragment::DeclaresSetByCallerTag(
	FGameplayTag DataTag) const
{
	return EffectRecipe.SetByCallers.ContainsByPredicate(
		[DataTag](const FPDSetByCallerValueStruct& Value)
		{
			return Value.DataTag.MatchesTagExact(DataTag);
		});
}

void UPDApplyGameplayEffectFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
	for (const FPDSetByCallerValueStruct& Value : EffectRecipe.SetByCallers)
	{
		if (Value.DataTag.IsValid())
		{
			OutTags.AddTag(Value.DataTag);
		}
	}
}

bool UPDApplyGameplayEffectFragment::SupportsDeferredExecution() const
{
	return !bTrackUntilAbilityEnds;
}

bool UPDApplyGameplayEffectFragment::PrepareDeferredExecution(
	const FPDActionExecutionContext& Context,
	FString& OutError)
{
	DeferredEffectSpec = FGameplayEffectSpecHandle();
	if (!SupportsDeferredExecution())
	{
		OutError = TEXT("GA 종료까지 추적하는 Gameplay Effect는 지연 실행할 수 없습니다.");
		return false;
	}

	if (!Validate(OutError))
	{
		return false;
	}

	UAbilitySystemComponent* SourceAbilitySystem = Context.SourceAbilitySystem;
	if (!SourceAbilitySystem || !Context.IsAuthoritative() ||
		!IsValid(Context.SourceActor))
	{
		OutError = TEXT("Gameplay Effect Spec을 준비할 서버 Source 문맥이 없습니다.");
		return false;
	}

	FGameplayEffectContextHandle EffectContext =
		SourceAbilitySystem->MakeEffectContext();
	AActor* EffectCauser = GetTypedOuter<AActor>();
	EffectContext.AddInstigator(
		Context.SourceActor,
		EffectCauser ? EffectCauser : Context.SourceActor);
	EffectContext.AddSourceObject(
		Context.EffectSourceObject ? Context.EffectSourceObject : this);

	DeferredEffectSpec = SourceAbilitySystem->MakeOutgoingSpec(
		EffectRecipe.EffectClass,
		EffectRecipe.EffectLevel,
		EffectContext);
	if (!DeferredEffectSpec.IsValid())
	{
		OutError = TEXT("지연 실행할 Gameplay Effect Spec 생성에 실패했습니다.");
		return false;
	}

	DeferredEffectSpec.Data->DynamicGrantedTags.AppendTags(
		EffectRecipe.DynamicGrantedTags);
	for (const FPDSetByCallerValueStruct& Value : EffectRecipe.SetByCallers)
	{
		DeferredEffectSpec.Data->SetSetByCallerMagnitude(
			Value.DataTag,
			Value.Magnitude);
	}

	OutError.Reset();
	return true;
}

bool UPDApplyGameplayEffectFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.SourceAbilitySystem ||
		!Context.ResolveScopedAbilitySystem(ApplicationScope))
	{
		OutError = TEXT("Gameplay Effect를 적용할 Source 또는 Target ASC가 없습니다.");
		return false;
	}

	if (!Context.IsAuthoritative())
	{
		OutError = TEXT("Gameplay Effect 적용 권한이 없습니다.");
		return false;
	}

	if (!Context.Ability && !DeferredEffectSpec.IsValid())
	{
		OutError = TEXT("지연 실행할 Gameplay Effect Spec이 준비되지 않았습니다.");
		return false;
	}

	return true;
}

bool UPDApplyGameplayEffectFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	UAbilitySystemComponent* TargetAbilitySystem =
		Context.ResolveScopedAbilitySystem(ApplicationScope);
	if (Context.Ability)
	{
		return Context.Ability->ApplyGameplayEffectRecipe(
			EffectRecipe,
			Context,
			TargetAbilitySystem,
			bTrackUntilAbilityEnds);
	}

	if (!Context.SourceAbilitySystem || !TargetAbilitySystem ||
		!DeferredEffectSpec.IsValid() || !Context.IsAuthoritative())
	{
		return false;
	}

	if (TargetAbilitySystem == Context.SourceAbilitySystem)
	{
		Context.SourceAbilitySystem->ApplyGameplayEffectSpecToSelf(
			*DeferredEffectSpec.Data.Get());
	}
	else
	{
		Context.SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(
			*DeferredEffectSpec.Data.Get(),
			TargetAbilitySystem);
	}
	return true;
}
