#include "PADO/AbilitySystem/Struct/PDMontageHitLagConfigStruct.h"

bool FPDMontageHitLagConfigStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(PlayRateMultiplier) ||
		PlayRateMultiplier < 0.05f || PlayRateMultiplier >= 1.0f)
	{
		OutError = TEXT("PlayRateMultiplier는 0.05 이상 1 미만이어야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(Duration) || Duration <= 0.0f || Duration > 0.5f)
	{
		OutError = TEXT("Duration은 0초 초과 0.5초 이하여야 합니다.");
		return false;
	}

	return true;
}

float FPDMontageHitLagConfigStruct::ResolvePlayRate(float BasePlayRate) const
{
	return FMath::Max(0.01f, BasePlayRate * PlayRateMultiplier);
}
