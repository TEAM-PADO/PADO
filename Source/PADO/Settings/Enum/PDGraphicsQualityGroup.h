// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "PDGraphicsQualityGroup.generated.h"

/** 옵션 그래픽 탭에서 따로 고르는 세부 품질 항목이다. 엔진 Scalability 그룹과 하나씩 대응한다. */
UENUM(BlueprintType)
enum class EPDGraphicsQualityGroup : uint8
{
	ViewDistance		UMETA(DisplayName = "View Distance"),
	Shadow				UMETA(DisplayName = "Shadow"),
	GlobalIllumination	UMETA(DisplayName = "Global Illumination"),
	Reflection			UMETA(DisplayName = "Reflection"),
	AntiAliasing		UMETA(DisplayName = "Anti-Aliasing"),
	Texture				UMETA(DisplayName = "Texture"),
	Effects				UMETA(DisplayName = "Effects"),
	PostProcess			UMETA(DisplayName = "Post Process"),
	Foliage				UMETA(DisplayName = "Foliage"),
	Shading				UMETA(DisplayName = "Shading")
};

ENUM_RANGE_BY_FIRST_AND_LAST(EPDGraphicsQualityGroup, EPDGraphicsQualityGroup::ViewDistance, EPDGraphicsQualityGroup::Shading);
