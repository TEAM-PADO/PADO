// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "PDSettingsTab.generated.h"

/** 옵션 창 상단 탭이다. 선언 순서대로 탭 버튼을 만든다. */
UENUM(BlueprintType)
enum class EPDSettingsTab : uint8
{
	General			UMETA(DisplayName = "General"),
	Display			UMETA(DisplayName = "Display"),
	Audio			UMETA(DisplayName = "Audio"),
	Controls		UMETA(DisplayName = "Controls"),
	KeyBindings		UMETA(DisplayName = "Key Bindings"),
	Accessibility	UMETA(DisplayName = "Accessibility")
};

ENUM_RANGE_BY_FIRST_AND_LAST(EPDSettingsTab, EPDSettingsTab::General, EPDSettingsTab::Accessibility);
