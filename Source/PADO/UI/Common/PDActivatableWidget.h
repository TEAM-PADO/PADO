// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PDActivatableWidget.generated.h"

/**
 * PADO 화면 단위 위젯의 공용 부모다.
 * 활성화될 때 메뉴 입력 모드(커서 표시, 게임 입력 차단)를 요청한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
