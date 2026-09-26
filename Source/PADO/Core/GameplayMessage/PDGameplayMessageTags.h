// Copyright PADO. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * 게임플레이 메시지 라우터에서 사용하는 태그입니다.
 */
namespace PDGameplayMessageTags
{
	// 게임 진행 단계 기반의 Gameplay Message 코어 예시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameFlowPhaseChanged); // 게임 진행 단계가 변경되었음을 알리는 메시지 채널입니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GamePhaseWaiting);  // 게임 시작 전 대기 단계입니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GamePhasePlaying); // 게임이 진행 중인 단계입니다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GamePhaseResults); // 게임 결과를 처리하는 단계입니다. 

	
}
