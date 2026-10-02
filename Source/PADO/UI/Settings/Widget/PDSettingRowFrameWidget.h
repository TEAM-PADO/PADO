// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PDSettingRowFrameWidget.generated.h"

class UCommonTextBlock;

/**
 * 옵션 줄의 공통 틀이다. 줄 높이·배경·여백·항목 이름 글꼴을 이 WBP 한 곳에서 정한다.
 * 종류별 줄 WBP는 이 틀을 Frame_Row로 하나 놓고, 틀의 Named Slot(Slot_Value)에 조작 위젯을 넣는다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingRowFrameWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FText& InLabel);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Label;
};
