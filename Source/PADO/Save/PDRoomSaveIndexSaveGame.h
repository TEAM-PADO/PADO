// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PADO/Save/Struct/PDRoomSaveListEntry.h"
#include "PDRoomSaveIndexSaveGame.generated.h"

/**
 * 저장 슬롯 이름을 열거할 수 없는 SaveGame API를 보완하는 로컬 저장 방 목록입니다.
 * 이 파일에는 실제 진행 데이터가 아닌 목록 표시용 요약만 보관합니다.
 */
UCLASS()
class PADO_API UPDRoomSaveIndexSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 방 저장 ID의 목록 항목이 이미 있으면 위치를 반환하고, 없으면 INDEX_NONE을 반환합니다. */
	int32 FindListEntryIndex(const FString& RoomSaveId) const;

	/** 같은 방 ID의 기존 목록 항목을 교체하거나 새 항목을 추가합니다. */
	void UpsertListEntry(const FPDRoomSaveListEntry& ListEntry);

	/** 지정한 방 저장 ID의 목록 항목을 제거하고 제거 여부를 반환합니다. */
	bool RemoveListEntry(const FString& RoomSaveId);

	/** 목록 내 개별 항목과 중복 방 ID를 검사합니다. */
	bool Validate(FString& OutError) const;

	/** 실제 파일은 개별 UPDRoomSaveGame에 있고, 여기에는 목록 카드 정보만 있습니다. */
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "PADO|Save")
	TArray<FPDRoomSaveListEntry> RoomSaveListEntries;
};
