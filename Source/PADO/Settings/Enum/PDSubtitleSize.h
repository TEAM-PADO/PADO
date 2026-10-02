// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDSubtitleSize.generated.h"

/** 자막 글자 크기다. 자막 위젯이 이 값을 읽어 글꼴 크기를 고른다. */
UENUM(BlueprintType)
enum class EPDSubtitleSize : uint8
{
	Small	UMETA(DisplayName = "Small"),
	Medium	UMETA(DisplayName = "Medium"),
	Large	UMETA(DisplayName = "Large")
};
