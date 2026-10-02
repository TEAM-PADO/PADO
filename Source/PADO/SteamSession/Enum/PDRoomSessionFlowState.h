// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDRoomSessionFlowState.generated.h"

/**
 * 로컬 프로세스의 방 입·퇴장 흐름에서 현재 진행 중인 비동기 단계를 나타냅니다.
 * 서버 게임 규칙이나 복제 대상 상태가 아니며, 메뉴 UI가 입력을 잠글 때 사용합니다.
 */
UENUM(BlueprintType)
enum class EPDRoomSessionFlowState : uint8
{
	/** 실행 중인 방 흐름 요청이 없습니다. */
	Idle UMETA(DisplayName = "Idle"),

	/** 새 방의 저장 컨텍스트를 준비하고 있습니다. */
	PreparingNewRoom UMETA(DisplayName = "Preparing New Room"),

	/** Steam 리슨 세션 생성을 기다리고 있습니다. */
	CreatingSteamSession UMETA(DisplayName = "Creating Steam Session"),

	/** 공개 Steam 세션 목록을 검색하고 있습니다. */
	SearchingForSessions UMETA(DisplayName = "Searching For Sessions"),

	/** 선택하거나 초대로 받은 Steam 세션 참가를 기다리고 있습니다. */
	JoiningSteamSession UMETA(DisplayName = "Joining Steam Session"),

	/** Steam 세션 생성·참가 성공 뒤 인게임 맵으로 이동하고 있습니다. */
	TravellingToGame UMETA(DisplayName = "Travelling To Game"),

	/** 호스트 종료 전 현재 방 상태를 저장하고 있습니다. */
	SavingBeforeHostExit UMETA(DisplayName = "Saving Before Host Exit"),

	/** 로컬 Steam 세션을 종료하거나 나가고 있습니다. */
	DestroyingSession UMETA(DisplayName = "Destroying Session"),

	/** 세션 정리 성공 후 메인 메뉴 맵으로 이동하고 있습니다. */
	ReturningToMainMenu UMETA(DisplayName = "Returning To Main Menu")
};
