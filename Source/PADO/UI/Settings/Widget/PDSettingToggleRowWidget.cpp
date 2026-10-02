// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingToggleRowWidget.h"

#include "CommonTextBlock.h"
#include "Components/CheckBox.h"
#include "PADO/UI/Settings/ViewModel/PDSettingToggleViewModel.h"

UWidget* UPDSettingToggleRowWidget::GetFocusTarget() const
{
	return CheckBox_Value;
}

void UPDSettingToggleRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CheckBox_Value->OnCheckStateChanged.AddDynamic(this, &ThisClass::HandleCheckStateChanged);
}

void UPDSettingToggleRowWidget::RefreshFromItem()
{
	Super::RefreshFromItem();

	const UPDSettingToggleViewModel* ToggleItem = Cast<UPDSettingToggleViewModel>(GetItem());
	if (!ToggleItem)
	{
		return;
	}

	CheckBox_Value->SetIsChecked(ToggleItem->IsOn());
	Text_Value->SetText(ToggleItem->GetValueText());
}

void UPDSettingToggleRowWidget::HandleCheckStateChanged(const bool bIsChecked)
{
	if (IsRefreshing())
	{
		return;
	}

	if (UPDSettingToggleViewModel* ToggleItem = Cast<UPDSettingToggleViewModel>(GetItem()))
	{
		ToggleItem->SetIsOnFromUser(bIsChecked);
	}
}
