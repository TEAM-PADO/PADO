// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/Widget/PDSettingRowWidget.h"
#include "PDSettingToggleRowWidget.generated.h"

class UCheckBox;
class UCommonTextBlock;

/** 스위치(체크박스)와 켜기/끄기 문구로 상태를 고르는 옵션 줄이다. UPDSettingToggleViewModel 항목을 표시한다. */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingToggleRowWidget : public UPDSettingRowWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetFocusTarget() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void RefreshFromItem() override;

private:
	UFUNCTION()
	void HandleCheckStateChanged(bool bIsChecked);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_Value;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Value;
};
