#pragma once

#include "CoreMinimal.h"
#include "PDDeliveryType.generated.h"

/**
 * 현재 진행 중인 배달의 큰 분류입니다.
 * 일반 배달과 보스 배달은 보상 및 실패 규칙이 달라질 수 있으므로 명시적으로 구분합니다.
 */
UENUM(BlueprintType)
enum class EPDDeliveryType : uint8
{
	General UMETA(DisplayName = "General"),
	Boss UMETA(DisplayName = "Boss")
};
