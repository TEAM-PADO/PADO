#include "PADO/AbilitySystem/Struct/PDActionCooldownStruct.h"

bool FPDActionCooldownStruct::IsEnabled() const
{
	return CooldownTag.IsValid() && Duration > 0.0f;
}

bool FPDActionCooldownStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(Duration) || Duration < 0.0f)
	{
		OutError = TEXT("Cooldown Duration은 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	// 둘 중 하나만 채우면 조용히 아무 일도 일어나지 않는다. 저작 실수를 여기서 잡는다.
	if (Duration > 0.0f && !CooldownTag.IsValid())
	{
		OutError = TEXT(
			"Cooldown Duration이 있으면 점유할 CooldownTag도 지정해야 합니다.");
		return false;
	}

	if (CooldownTag.IsValid() && Duration <= 0.0f)
	{
		OutError = TEXT(
			"CooldownTag를 지정했으면 Duration은 0보다 커야 합니다.");
		return false;
	}

	return true;
}
