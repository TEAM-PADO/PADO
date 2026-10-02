// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingKeyBindingRowWidget.h"

#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/Settings/ViewModel/PDSettingKeyBindingViewModel.h"

UWidget* UPDSettingKeyBindingRowWidget::GetFocusTarget() const
{
	return Button_Key;
}

void UPDSettingKeyBindingRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Key->OnClicked().AddUObject(this, &ThisClass::HandleKeyClicked);
}

void UPDSettingKeyBindingRowWidget::RefreshFromItem()
{
	Super::RefreshFromItem();

	if (const UPDSettingKeyBindingViewModel* KeyItem = Cast<UPDSettingKeyBindingViewModel>(GetItem()))
	{
		Button_Key->SetButtonText(KeyItem->GetKeyText());
	}
}

void UPDSettingKeyBindingRowWidget::HandleKeyClicked()
{
	if (UPDSettingKeyBindingViewModel* KeyItem = Cast<UPDSettingKeyBindingViewModel>(GetItem()))
	{
		KeyItem->RequestKeyCapture();
	}
}
