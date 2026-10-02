// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "PADO/UI/Common/PDActivatableWidget.h"
#include "PDKeyCaptureWidget.generated.h"

class FPDKeyCaptureInputProcessor;
class UCommonTextBlock;

/**
 * 키 설정에서 새 키를 기다리는 창이다. 옵션 창의 대화상자 스택에 올린다.
 * 떠 있는 동안 모든 키보드·마우스 입력을 가로채고, 키 하나를 받으면 닫은 뒤 결과를 넘긴다. Esc는 취소다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDKeyCaptureWidget : public UPDActivatableWidget
{
	GENERATED_BODY()

public:
	UPDKeyCaptureWidget(const FObjectInitializer& ObjectInitializer);

	/** 안내할 동작 이름과 키를 받았을 때 실행할 함수를 지정한다. 취소하면 실행하지 않는다. */
	void Setup(const FText& ActionName, TFunction<void(const FKey&)> InOnKeyCaptured);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void StopCapture();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Prompt;

	TSharedPtr<FPDKeyCaptureInputProcessor> InputProcessor;
	TFunction<void(const FKey&)> OnKeyCaptured;
};
