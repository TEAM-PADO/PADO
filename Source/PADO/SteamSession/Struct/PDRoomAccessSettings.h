// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/SteamSession/Enum/PDRoomAccessPolicy.h"
#include "PDRoomAccessSettings.generated.h"

/** 방 참가 URL에서 비밀번호를 전달할 때 사용하는 옵션 키입니다. */
namespace PDRoomAccessOptions
{
	inline constexpr TCHAR PasswordKey[] = TEXT("PADOAccessCode");
}

/**
 * 새 Steam 방 생성 시 호스트가 선택하는 입장 설정입니다.
 * 비밀번호는 생성 요청에만 사용되며 SaveGame 또는 Steam 세션 메타데이터에 저장하지 않습니다.
 */
USTRUCT(BlueprintType)
struct FPDRoomAccessSettings
{
	GENERATED_BODY()

	/** 방에 적용할 입장 정책입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Access")
	EPDRoomAccessPolicy AccessPolicy = EPDRoomAccessPolicy::Public;

	/** FriendsOrPassword 정책에서만 사용하는 4~8자리 숫자 방 코드입니다. 앞의 0도 코드의 일부로 처리됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Access", meta = (EditCondition = "AccessPolicy == EPDRoomAccessPolicy::FriendsOrPassword", EditConditionHides))
	FString Password;

	/** @return 현재 설정이 방 생성에 사용할 수 있으면 true입니다. 실패 이유는 OutError에 기록합니다. */
	bool Validate(FString& OutError) const;

	/** @return 숫자 4~8자리 방 코드면 true입니다. 생성·참가·서버 검증이 같은 규칙을 사용합니다. */
	static bool ValidatePassword(const FString& Password, FString& OutError);
};
