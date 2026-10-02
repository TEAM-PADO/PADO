// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDRoomSaveListEntry.generated.h"

/**
 * 메인 메뉴가 저장 방 하나를 표시하는 데 필요한 최소 목록 정보입니다.
 * 실제 방 진행 데이터는 UPDRoomSaveGame에만 보관합니다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDRoomSaveListEntry
{
	GENERATED_BODY()

	/** 새 게임을 만들 때 생성되며, 방 이름 변경과 Steam 세션 재생성에도 바뀌지 않는 영구 식별자입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "PADO|Save")
	FString RoomSaveId;

	/** 이 저장 방을 생성한 호스트의 플랫폼 고유 ID 문자열입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "PADO|Save")
	FString HostPlatformUserId;

	/** UI에 표시하는 방 이름입니다. 저장 슬롯 파일명이나 권한 판정에는 사용하지 않습니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "PADO|Save")
	FString RoomDisplayName;

	/** 저장본의 방 공용 진행 요약입니다. 현재는 완료한 일반 배달 수만 포함합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "PADO|Save")
	int32 CompletedGeneralDeliveryCount = 0;

	/** 저장 목록에 표시하기에 안전한 최소 식별자·진행 값인지 검사합니다. */
	bool Validate(FString& OutError) const;
};
