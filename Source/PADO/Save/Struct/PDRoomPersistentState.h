// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDRoomPersistentState.generated.h"

/**
 * 방 전체의 영속 가능한 진행 상태입니다.
 * 이벤트별 저장 시점은 GameMode가 결정하고, 이 구조는 그 결정 결과만 값으로 보관합니다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDRoomPersistentState
{
	GENERATED_BODY()

	/** 완료한 일반 배달의 누적 수입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "PADO|Save")
	int32 CompletedGeneralDeliveryCount = 0;

	/** 현재 저장 가능한 값이 유효한지 검사합니다. 실패 사유는 영어로 반환합니다. */
	bool Validate(FString& OutError) const;
};
