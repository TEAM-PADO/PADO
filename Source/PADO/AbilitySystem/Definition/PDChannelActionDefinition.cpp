#include "PADO/AbilitySystem/Definition/PDChannelActionDefinition.h"

#include "PADO/AbilitySystem/Ability/PDGA_ChannelAction.h"

TSubclassOf<UPDGA_Base> UPDChannelActionDefinition::GetAbilityClass() const
{
	return UPDGA_ChannelAction::StaticClass();
}

bool UPDChannelActionDefinition::ValidateLifecycle(FString& OutError) const
{
	// 실행 시점을 몽타주의 Notify에서 받으므로 몽타주가 없으면 영원히 아무것도
	// 실행하지 못한다.
	if (!IsValid(ActionMontage.Montage))
	{
		OutError = TEXT(
			"Channel Action에는 실행 시점을 담을 Montage가 필요합니다.");
		return false;
	}

	FString LoopingCueError;
	if (!LoopingCue.Validate(LoopingCueError))
	{
		OutError = FString::Printf(
			TEXT("LoopingCue가 유효하지 않습니다: %s"),
			*LoopingCueError);
		return false;
	}

	return true;
}
