// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDControlsSettings.generated.h"

class UInputMappingContext;

/**
 * 옵션 키 설정에 쓰는 프로젝트 설정이다.
 * 여기 지정한 입력 매핑 컨텍스트를 Enhanced Input 사용자 설정에 등록해, 메인 메뉴에서도 키를 바꿀 수 있게 한다.
 * 실제로 바꿀 수 있는 키는 입력 에셋의 Player Mappable Key Settings에 이름이 지정된 매핑뿐이다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "PADO Controls"))
class PADO_API UPDControlsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 키 설정 탭에 보여 줄 입력 매핑 컨텍스트다. 컨텍스트와 매핑 순서대로 줄을 만든다. */
	UPROPERTY(Config, EditAnywhere, Category = "Key Bindings")
	TArray<TSoftObjectPtr<UInputMappingContext>> KeyBindingMappingContexts;
};
