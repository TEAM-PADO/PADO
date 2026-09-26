// Copyright PADO. All Rights Reserved.

#include "PDGameplayMessageTags.h"

namespace PDGameplayMessageTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameFlowPhaseChanged, "Message.Core.GameFlow.PhaseChanged", "게임 진행 단계 변경 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GamePhaseWaiting, "GamePhase.Waiting", "게임 시작 전 대기 단계입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GamePhasePlaying, "GamePhase.Playing", "게임이 진행 중인 단계입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GamePhaseResults, "GamePhase.Results", "게임 결과를 처리하는 단계입니다.");
}
