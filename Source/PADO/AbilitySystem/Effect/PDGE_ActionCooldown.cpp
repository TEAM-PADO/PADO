#include "PADO/AbilitySystem/Effect/PDGE_ActionCooldown.h"

#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"

UPDGE_ActionCooldown::UPDGE_ActionCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat CooldownDuration;
	CooldownDuration.DataTag = TAG_PD_Data_Cooldown_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(CooldownDuration);
}
