#include "PADO/AbilitySystem/Ability/PDGA_Base.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Effect/PDGE_ActionCooldown.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PADO/AbilitySystem/Struct/PDGameplayEffectRecipeStruct.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDActionHook, Log, All);

UPDGA_Base::UPDGA_Base()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UPDGA_Base::ValidateDefinitionContract(
	const UPDAbilityDefinition& Definition,
	FString& OutError) const
{
	OutError.Reset();
	for (const FPDActionHookStruct& Hook : Definition.ActionHooks)
	{
		if (!SupportedActionHooks.HasTagExact(Hook.HookTag))
		{
			OutError = FString::Printf(
				TEXT("Ability '%s'가 Action Hook '%s'를 지원하지 않습니다."),
				*GetClass()->GetName(),
				*Hook.HookTag.ToString());
			return false;
		}
	}

	for (const FGameplayTag& RequiredHook : RequiredActionHooks)
	{
		if (!Definition.FindActionHook(RequiredHook))
		{
			OutError = FString::Printf(
				TEXT("Ability '%s'에 필요한 Action Hook '%s'가 없습니다."),
				*GetClass()->GetName(),
				*RequiredHook.ToString());
			return false;
		}
	}

	return true;
}

bool UPDGA_Base::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	const UPDAbilityDefinition* Definition = nullptr;
	if (!ResolveDefinitionForSpec(Handle, ActorInfo, Definition))
	{
		return false;
	}

	return Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags);
}

bool UPDGA_Base::CheckCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// CDO에 CooldownGameplayEffectClass를 둔 파생 GA도 계속 동작하게 둔다.
	if (!Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	const UPDAbilityDefinition* Definition = nullptr;
	if (!ResolveDefinitionForSpec(Handle, ActorInfo, Definition) ||
		!Definition->ActionCooldown.IsEnabled())
	{
		return true;
	}

	const UAbilitySystemComponent* AbilitySystem =
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!AbilitySystem ||
		!AbilitySystem->HasMatchingGameplayTag(
			Definition->ActionCooldown.CooldownTag))
	{
		return true;
	}

	const FGameplayTag& FailCooldownTag =
		UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
	if (OptionalRelevantTags && FailCooldownTag.IsValid())
	{
		OptionalRelevantTags->AddTag(FailCooldownTag);
	}
	return false;
}

void UPDGA_Base::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);

	const UPDAbilityDefinition* Definition = nullptr;
	if (!ResolveDefinitionForSpec(Handle, ActorInfo, Definition) ||
		!Definition->ActionCooldown.IsEnabled())
	{
		return;
	}

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(
		Handle,
		ActorInfo,
		ActivationInfo,
		UPDGE_ActionCooldown::StaticClass(),
		GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpec.IsValid())
	{
		return;
	}

	// 점유할 슬롯과 지속시간을 Spec에 실어 보낸다. 그래서 Action마다 GE 에셋을
	// 따로 만들지 않아도 된다.
	CooldownSpec.Data->DynamicGrantedTags.AddTag(
		Definition->ActionCooldown.CooldownTag);
	CooldownSpec.Data->SetSetByCallerMagnitude(
		TAG_PD_Data_Cooldown_Duration,
		Definition->ActionCooldown.Duration);
	ApplyGameplayEffectSpecToOwner(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownSpec);
}

void UPDGA_Base::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	const UPDAbilityDefinition* Definition = nullptr;
	if (!ResolveDefinitionForSpec(Handle, ActorInfo, Definition))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveDefinition = const_cast<UPDAbilityDefinition*>(Definition);
	TrackedActiveEffects.Reset();
	ExecutionChargeAlpha = 1.0f;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPDGA_Base::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() &&
		ActorInfo->AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		for (const FPDTrackedActiveEffect& TrackedEffect : TrackedActiveEffects)
		{
			if (TrackedEffect.TargetAbilitySystem.IsValid() &&
				TrackedEffect.EffectHandle.IsValid())
			{
				TrackedEffect.TargetAbilitySystem->RemoveActiveGameplayEffect(
					TrackedEffect.EffectHandle);
			}
		}
	}

	TrackedActiveEffects.Reset();
	ActiveDefinition = nullptr;
	ExecutionChargeAlpha = 1.0f;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UPDGA_Base::AddSupportedActionHook(FGameplayTag HookTag)
{
	if (HookTag.IsValid())
	{
		SupportedActionHooks.AddTag(HookTag);
	}
}

void UPDGA_Base::AddRequiredActionHook(FGameplayTag HookTag)
{
	if (HookTag.IsValid())
	{
		SupportedActionHooks.AddTag(HookTag);
		RequiredActionHooks.AddTag(HookTag);
	}
}

bool UPDGA_Base::ExecuteActionHook(
	FGameplayTag HookTag,
	UAbilitySystemComponent* TargetAbilitySystem,
	AActor* TargetActor,
	const FHitResult* HitResult)
{
	UAbilitySystemComponent* SourceAbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	const FPDActionHookStruct* Hook = ActiveDefinition
		? ActiveDefinition->FindActionHook(HookTag)
		: nullptr;
	if (!SourceAbilitySystem || !Hook)
	{
		return false;
	}

	FPDActionExecutionContext Context;
	Context.Ability = this;
	Context.SourceAbilitySystem = SourceAbilitySystem;
	Context.TargetAbilitySystem = TargetAbilitySystem;
	Context.SourceActor = GetAvatarActorFromActorInfo();
	Context.TargetActor = TargetActor;
	Context.EffectSourceObject = GetCurrentSourceObject();
	Context.InputChargeAlpha = ExecutionChargeAlpha;
	if (HitResult)
	{
		Context.HitResult = *HitResult;
		Context.bHasHitResult = true;
	}

	TArray<const UPDActionFragment*, TInlineAllocator<8>> ExecutableFragments;
	for (const UPDActionFragment* Fragment : Hook->Fragments)
	{
		if (!IsValid(Fragment))
		{
			UE_LOG(
				LogPDActionHook,
				Warning,
				TEXT("Hook '%s'에 비어 있는 Fragment가 있어 실행을 중단했습니다."),
				*HookTag.ToString());
			return false;
		}

		FString ExecutionError;
		if (Fragment->CanExecute(Context, ExecutionError))
		{
			ExecutableFragments.Add(Fragment);
		}
		else if (Fragment->bRequired)
		{
			UE_LOG(
				LogPDActionHook,
				Warning,
				TEXT("Hook '%s'의 필수 Fragment '%s'가 대상 '%s'에서 실행 조건을 ")
				TEXT("만족하지 못해 Hook 전체를 취소했습니다: %s"),
				*HookTag.ToString(),
				*GetNameSafe(Fragment),
				*GetNameSafe(TargetActor),
				*ExecutionError);
			return false;
		}
	}

	for (const UPDActionFragment* Fragment : ExecutableFragments)
	{
		if (!Fragment->Execute(Context) && Fragment->bRequired)
		{
			UE_LOG(
				LogPDActionHook,
				Warning,
				TEXT("Hook '%s'의 필수 Fragment '%s'가 대상 '%s'에서 실행에 실패해 ")
				TEXT("남은 Fragment를 중단했습니다."),
				*HookTag.ToString(),
				*GetNameSafe(Fragment),
				*GetNameSafe(TargetActor));
			return false;
		}
	}

	return true;
}

const UPDAbilityDefinition* UPDGA_Base::GetActiveDefinition() const
{
	return ActiveDefinition;
}

void UPDGA_Base::SetExecutionChargeAlpha(float ChargeAlpha)
{
	ExecutionChargeAlpha = FMath::Clamp(ChargeAlpha, 0.0f, 1.0f);
}

float UPDGA_Base::GetExecutionChargeAlpha() const
{
	return ExecutionChargeAlpha;
}

bool UPDGA_Base::ApplyGameplayEffectRecipe(
	const FPDGameplayEffectRecipeStruct& Recipe,
	const FPDActionExecutionContext& Context,
	UAbilitySystemComponent* TargetAbilitySystem,
	bool bTrackUntilAbilityEnds)
{
	UAbilitySystemComponent* SourceAbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	if (Context.Ability != this ||
		Context.SourceAbilitySystem != SourceAbilitySystem ||
		!TargetAbilitySystem ||
		!Context.IsAuthoritative())
	{
		return false;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeEffectSpec(Recipe);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	FActiveGameplayEffectHandle ActiveHandle;
	if (TargetAbilitySystem == SourceAbilitySystem)
	{
		ActiveHandle = SourceAbilitySystem->ApplyGameplayEffectSpecToSelf(
			*SpecHandle.Data.Get());
	}
	else
	{
		ActiveHandle = SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(
			*SpecHandle.Data.Get(),
			TargetAbilitySystem);
	}

	if (bTrackUntilAbilityEnds && ActiveHandle.IsValid())
	{
		FPDTrackedActiveEffect& TrackedEffect =
			TrackedActiveEffects.AddDefaulted_GetRef();
		TrackedEffect.TargetAbilitySystem = TargetAbilitySystem;
		TrackedEffect.EffectHandle = ActiveHandle;
	}

	return true;
}

bool UPDGA_Base::ResolveDefinitionForSpec(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const UPDAbilityDefinition*& OutDefinition,
	FString* OutError) const
{
	OutDefinition = nullptr;
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("AbilityActorInfo 또는 AbilitySystemComponent가 없습니다.");
		}
		return false;
	}

	const FGameplayAbilitySpec* Spec =
		ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	if (!Spec || !Spec->SourceObject.IsValid())
	{
		if (OutError)
		{
			*OutError = TEXT("정확한 AbilitySpec 또는 SourceObject를 찾지 못했습니다.");
		}
		return false;
	}

	// Grant 시점에 검증을 마쳤고 Grant 중에는 Definition을 교체할 수 없다.
	if (!UPDAbilitySystemComponent::GetDefinitionFromSource(
		Spec->SourceObject.Get(),
		OutDefinition,
		OutError))
	{
		return false;
	}

	if (OutDefinition->GetAbilityClass() != GetClass())
	{
		if (OutError)
		{
			*OutError = TEXT("Definition에 고정된 Ability와 실행 중인 Ability 클래스가 다릅니다.");
		}
		return false;
	}

	return true;
}

FGameplayEffectSpecHandle UPDGA_Base::MakeEffectSpec(
	const FPDGameplayEffectRecipeStruct& Recipe) const
{
	UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystem || !Recipe.EffectClass)
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddSourceObject(GetCurrentSourceObject());

	FGameplayEffectSpecHandle SpecHandle = AbilitySystem->MakeOutgoingSpec(
		Recipe.EffectClass,
		Recipe.EffectLevel,
		Context);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	SpecHandle.Data->DynamicGrantedTags.AppendTags(Recipe.DynamicGrantedTags);
	// Recipe가 자기 값을 직접 들고 있으므로 같은 태그라도 GE마다 다른 값을 넣을 수 있다.
	for (const FPDSetByCallerValueStruct& Value : Recipe.SetByCallers)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Value.DataTag, Value.Magnitude);
	}

	return SpecHandle;
}
