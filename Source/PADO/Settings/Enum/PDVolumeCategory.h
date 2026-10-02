// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "PDVolumeCategory.generated.h"

/** 옵션에서 따로 조절하는 음량 분류다. 분류마다 Sound Class 하나를 PADO Audio 프로젝트 설정에 연결한다. */
UENUM(BlueprintType)
enum class EPDVolumeCategory : uint8
{
	Master	UMETA(DisplayName = "Master"),
	Music	UMETA(DisplayName = "Music"),
	Effects	UMETA(DisplayName = "Effects"),
	UI		UMETA(DisplayName = "UI")
};

ENUM_RANGE_BY_FIRST_AND_LAST(EPDVolumeCategory, EPDVolumeCategory::Master, EPDVolumeCategory::UI);
