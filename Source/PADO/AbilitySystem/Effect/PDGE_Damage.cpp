#include "PADO/AbilitySystem/Effect/PDGE_Damage.h"

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
