// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PADO/Save/Struct/PDRoomPersistentState.h"
#include "PADO/Save/Struct/PDRoomSaveListEntry.h"
#include "PDRoomSaveGame.generated.h"

/**
 * 호스트 한 명이 소유하는 방 하나의 실제 디스크 저장본입니다.
 * 월드 Actor나 PlayerState 포인터를 저장하지 않고, 다시 구성 가능한 값만 보관합니다.
 */
UCLASS()
class PADO_API UPDRoomSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 이 클래스가 기록하는 현재 저장 데이터 형식 버전입니다. */
	static constexpr int32 CurrentSchemaVersion = 1;

	/** 저장 데이터의 유효성과 지원하는 스키마 버전을 검사합니다. */
	bool Validate(FString& OutError) const;

	/** 메인 메뉴 목록에 표시할 최소 정보 사본을 생성합니다. */
	FPDRoomSaveListEntry CreateListEntry() const;

	/** 저장 데이터 형식 버전입니다. 현재 코드보다 높은 버전의 파일은 불러오지 않습니다. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "PADO|Save")
	int32 SaveSchemaVersion = CurrentSchemaVersion;

	/** Steam 세션 ID와 별개인 영구 방 저장 식별자입니다. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "PADO|Save")
	FString RoomSaveId;

	/** 이 저장본을 만든 호스트의 플랫폼 고유 ID 문자열입니다. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "PADO|Save")
	FString HostPlatformUserId;

	/** 저장 목록에 표시할 방 이름입니다. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "PADO|Save")
	FString RoomDisplayName;

	/** 방 전체의 영속 가능한 진행 상태입니다. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "PADO|Save")
	FPDRoomPersistentState PersistentState;
};
