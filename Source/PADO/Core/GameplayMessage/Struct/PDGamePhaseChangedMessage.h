// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PDGamePhaseChangedMessage.generated.h"

/**
 * 게임 진행 단계 변경 메시지의 Payload입니다.
 *
 * `Message.Core.GameFlow.PhaseChanged` 채널에는 이 구조체만 전송해야 합니다.
 * 첫 단계 설정 메시지에서는 PreviousPhase가 유효하지 않은 태그일 수 있습니다.
 */
USTRUCT(BlueprintType)
struct PADO_API FPDGamePhaseChangedMessage
{
	GENERATED_BODY()

	/** 변경 전 게임 진행 단계입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Message", meta = (Categories = "GamePhase"))
	FGameplayTag PreviousPhase;

	/** 변경 후 게임 진행 단계입니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Message", meta = (Categories = "GamePhase"))
	FGameplayTag CurrentPhase;
};
