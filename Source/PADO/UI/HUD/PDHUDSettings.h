// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDHUDSettings.generated.h"

class UPDPerformanceOverlayWidget;

/** 게임 화면 위에 항상 떠 있는 UI의 프로젝트 설정이다. 프로젝트 설정 > PADO HUD에서 지정한다. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "PADO HUD"))
class PADO_API UPDHUDSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 옵션의 "성능 정보 표시"를 켰을 때 화면 구석에 띄울 WBP다. */
	UPROPERTY(Config, EditAnywhere, Category = "Performance Info")
	TSoftClassPtr<UPDPerformanceOverlayWidget> PerformanceOverlayClass;

	/** 성능 정보가 다른 UI 위에 보이도록 주는 화면 순서다. */
	UPROPERTY(Config, EditAnywhere, Category = "Performance Info")
	int32 PerformanceOverlayZOrder = 1000;
};
