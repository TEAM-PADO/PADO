#include "PADO/AbilitySystem/Effect/PDGE_MoveSpeedMultiplier.h"

#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"

UPDGE_MoveSpeedMultiplier::UPDGE_MoveSpeedMultiplier()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo& SpeedModifier = Modifiers.AddDefaulted_GetRef();
	SpeedModifier.Attribute = UPDMovementAttributeSet::GetMoveSpeedAttribute();

	// Multiply (Additive)는 감소량을 더한다. 0.5 두 개가 0이 되어 버린다.
	// Compound는 실제로 곱하므로 0.5 두 개가 0.25가 된다.
	SpeedModifier.ModifierOp = EGameplayModOp::MultiplyCompound;

	FSetByCallerFloat SpeedMultiplier;
	SpeedMultiplier.DataTag = TAG_PD_Data_MoveSpeed_Multiplier;
	SpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SpeedMultiplier);
}
