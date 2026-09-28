#include "PADO/AbilitySystem/Struct/PDLoopingCueStruct.h"

bool FPDLoopingCueStruct::IsEnabled() const
{
	return CueTag.IsValid();
}

bool FPDLoopingCueStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!CueTag.IsValid())
	{
		// 비워 두는 것은 정상이다. Looping Cue를 쓰지 않는다는 뜻이다.
		return true;
	}

	const FGameplayTag GameplayCueRoot =
		FGameplayTag::RequestGameplayTag(TEXT("GameplayCue"), false);
	if (!GameplayCueRoot.IsValid() || !CueTag.MatchesTag(GameplayCueRoot))
	{
		OutError = TEXT("CueTag는 GameplayCue 하위 태그여야 합니다.");
		return false;
	}

	return true;
}
