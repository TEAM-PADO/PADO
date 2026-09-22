#include "PADO/AbilitySystem/Struct/PDGameplayEffectRecipeStruct.h"

#include "GameplayEffect.h"

bool FPDGameplayEffectRecipeStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!EffectClass)
	{
		OutError = TEXT("EffectClass가 비어 있습니다.");
		return false;
	}

	if (!FMath::IsFinite(EffectLevel) || EffectLevel < 0.0f)
	{
		OutError = TEXT("EffectLevel은 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	TSet<FGameplayTag> SeenDataTags;
	for (int32 Index = 0; Index < SetByCallers.Num(); ++Index)
	{
		const FPDSetByCallerValueStruct& Value = SetByCallers[Index];
		FString ValueError;
		if (!Value.Validate(ValueError))
		{
			OutError = FString::Printf(
				TEXT("SetByCallers[%d]가 유효하지 않습니다: %s"),
				Index,
				*ValueError);
			return false;
		}

		if (SeenDataTags.Contains(Value.DataTag))
		{
			OutError = FString::Printf(
				TEXT("SetByCaller 태그 '%s'가 중복됩니다."),
				*Value.DataTag.ToString());
			return false;
		}
		SeenDataTags.Add(Value.DataTag);
	}

	return true;
}
