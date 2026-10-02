// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/Widget/PDSettingRowWidget.h"
#include "PDSettingSliderRowWidget.generated.h"

class UCommonTextBlock;
class USlider;

/** 슬라이더와 현재 값 문구로 수치를 고르는 옵션 줄이다. UPDSettingSliderViewModel 항목을 표시한다. */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingSliderRowWidget : public UPDSettingRowWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetFocusTarget() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void RefreshFromItem() override;

private:
	UFUNCTION()
	void HandleSliderValueChanged(float Value);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Value;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Value;
};
