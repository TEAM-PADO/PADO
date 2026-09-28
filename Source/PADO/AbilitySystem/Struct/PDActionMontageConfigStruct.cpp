#include "PADO/AbilitySystem/Struct/PDActionMontageConfigStruct.h"

#include "Animation/AnimMontage.h"

bool FPDActionMontageConfigStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(PlayRate) || PlayRate <= 0.0f)
	{
		OutError = TEXT("PlayRate는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!IsValid(Montage))
	{
		if (!StartSection.IsNone())
		{
			OutError = TEXT("Montage 없이 StartSection만 지정할 수 없습니다.");
			return false;
		}
		return true;
	}

	if (!StartSection.IsNone() && Montage->GetSectionIndex(StartSection) == INDEX_NONE)
	{
		OutError = FString::Printf(
			TEXT("Montage '%s'에 StartSection '%s'가 없습니다."),
			*Montage->GetName(),
			*StartSection.ToString());
		return false;
	}

	return true;
}
