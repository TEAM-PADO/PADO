// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PADO/Settings/Enum/PDVolumeCategory.h"
#include "PDAudioSettings.generated.h"

class USoundClass;
class USoundMix;

/**
 * 옵션 음량을 실제 소리에 연결하는 프로젝트 설정이다.
 * 분류별 Sound Class에 UserVolumeSoundMix의 음량 덮어쓰기를 걸어 개인 음량을 반영한다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "PADO Audio"))
class PADO_API UPDAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 개인 음량 전용 Sound Mix다. 다른 효과를 넣지 않은 빈 에셋을 지정한다. */
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundMix> UserVolumeSoundMix;

	/** 음량 분류별 Sound Class다. Master에는 나머지 분류의 부모 클래스를 지정한다. */
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TMap<EPDVolumeCategory, TSoftObjectPtr<USoundClass>> VolumeSoundClasses;
};
