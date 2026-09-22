#include "PADO/AbilitySystem/Definition/PDChannelActionDefinition.h"

#include "PADO/AbilitySystem/Ability/PDGA_ChannelAction.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"

TSubclassOf<UPDGA_Base> UPDChannelActionDefinition::GetAbilityClass() const
{
	return UPDGA_ChannelAction::StaticClass();
}

bool UPDChannelActionDefinition::ValidateLifecycle(FString& OutError) const
{
	/**
	 * MontageEvent는 실행 시점을 몽타주의 Notify에서 받으므로 몽타주가 없으면
	 * 영원히 아무것도 실행하지 못한다. FixedInterval은 서버 타이머가 시점을 만들고
	 * Instant Targeting만 허용하므로 몽타주는 연출일 뿐이라 없어도 된다.
	 */
	if (ExecutionMode == EPDChannelExecutionMode::MontageEvent &&
		!IsValid(ActionMontage.Montage))
	{
		OutError = TEXT(
			"Montage Event Channel Action에는 실행 시점을 담을 Montage가 필요합니다.");
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

	if (ExecutionMode == EPDChannelExecutionMode::FixedInterval)
	{
		if (!ActionTargeting ||
			!ActionTargeting->IsA<UPDInstantActionTargeting>())
		{
			OutError = TEXT(
				"FixedInterval Channel Action에는 Instant Targeting이 필요합니다.");
			return false;
		}

		if (!FMath::IsFinite(PulseInterval) ||
			PulseInterval < MinimumPulseInterval)
		{
			OutError = FString::Printf(
				TEXT("FixedInterval의 PulseInterval은 %.2f초 이상의 유한한 값이어야 합니다."),
				MinimumPulseInterval);
			return false;
		}
	}

	return true;
}
