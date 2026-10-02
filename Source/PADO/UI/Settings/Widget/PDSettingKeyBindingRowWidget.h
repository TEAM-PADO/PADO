// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/Widget/PDSettingRowWidget.h"
#include "PDSettingKeyBindingRowWidget.generated.h"

class UPDButtonBase;

/** 동작 이름과 지금 키를 보여 주고, 키 버튼을 누르면 새 키를 기다리는 옵션 줄이다. UPDSettingKeyBindingViewModel 항목을 표시한다. */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingKeyBindingRowWidget : public UPDSettingRowWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetFocusTarget() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void RefreshFromItem() override;

private:
	void HandleKeyClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Key;
};
