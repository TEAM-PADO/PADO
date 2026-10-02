// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDRoomAccessPolicy.generated.h"

/**
 * Steam 방의 서버 입장 검증 규칙입니다.
 * 실제 입장 허용 여부는 서버 APDGameMode의 PreLogin에서 최종 판정합니다.
 */
UENUM(BlueprintType)
enum class EPDRoomAccessPolicy : uint8
{
	/** 모든 Steam 사용자가 세션 목록 또는 초대를 통해 입장할 수 있습니다. */
	Public UMETA(DisplayName = "Public"),

	/** 호스트의 Steam 친구이거나 올바른 비밀번호를 제출한 사용자만 입장할 수 있습니다. */
	FriendsOrPassword UMETA(DisplayName = "Friends Or Password")
};
