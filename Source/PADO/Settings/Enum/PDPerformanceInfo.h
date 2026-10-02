// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDPerformanceInfo.generated.h"

/** 화면 구석에 보여 줄 성능 정보다. 성능 정보 오버레이가 이 값을 읽는다. */
UENUM(BlueprintType)
enum class EPDPerformanceInfo : uint8
{
	Off			UMETA(DisplayName = "Off"),
	Fps			UMETA(DisplayName = "FPS"),
	FpsAndPing	UMETA(DisplayName = "FPS and Ping")
};
