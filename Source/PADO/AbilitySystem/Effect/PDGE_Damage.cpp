#include "PADO/AbilitySystem/Effect/PDGE_Damage.h"

#include "AbilitySystemComponent.h"
#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"

UPDGE_Damage::UPDGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& DamageModifier = Modifiers.AddDefaulted_GetRef();
	DamageModifier.Attribute = UPDHealthAttributeSet::GetDamageAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat DamageAmount;
	DamageAmount.DataTag = TAG_PD_Data_Damage;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageAmount);
}

bool UPDGE_Damage::ApplyDamage(
	UAbilitySystemComponent& Source,
	UAbilitySystemComponent& Target,
	float Damage,
	AActor* EffectCauser)
{
	const AActor* SourceOwner = Source.GetOwnerActor();
	if (Damage <= 0.0f || !SourceOwner || !SourceOwner->HasAuthority())
	{
		return false;
	}

	// 아이템 행동과 같은 Effect Context 규칙이다(GASItemActionSystem.md).
	FGameplayEffectContextHandle EffectContext = Source.MakeEffectContext();
	EffectContext.AddInstigator(Source.GetOwnerActor(), EffectCauser);

	const FGameplayEffectSpecHandle Spec =
		Source.MakeOutgoingSpec(StaticClass(), 1.0f, EffectContext);
	if (!Spec.IsValid())
	{
		return false;
	}

	Spec.Data->SetSetByCallerMagnitude(TAG_PD_Data_Damage, Damage);
	const FActiveGameplayEffectHandle Handle = &Source == &Target
		? Source.ApplyGameplayEffectSpecToSelf(*Spec.Data.Get())
		: Source.ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), &Target);
	return Handle.WasSuccessfullyApplied();
}
