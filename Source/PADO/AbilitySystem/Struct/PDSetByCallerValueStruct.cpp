#include "PADO/AbilitySystem/Struct/PDSetByCallerValueStruct.h"

bool FPDSetByCallerValueStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!DataTag.IsValid())
	{
		OutError = TEXT("SetByCaller DataTag가 비어 있습니다.");
		return false;
	}

	if (!FMath::IsFinite(Magnitude))
	{
		OutError = FString::Printf(
			TEXT("SetByCaller '%s'의 Magnitude가 유한한 수가 아닙니다."),
			*DataTag.ToString());
		return false;
	}

	return true;
}
