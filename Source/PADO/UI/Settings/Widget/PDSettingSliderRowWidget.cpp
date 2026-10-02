// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingSliderRowWidget.h"

#include "CommonTextBlock.h"
#include "Components/Slider.h"
#include "PADO/UI/Settings/ViewModel/PDSettingSliderViewModel.h"

UWidget* UPDSettingSliderRowWidget::GetFocusTarget() const
{
	return Slider_Value;
}

void UPDSettingSliderRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Slider_Value->OnValueChanged.AddDynamic(this, &ThisClass::HandleSliderValueChanged);
}

void UPDSettingSliderRowWidget::RefreshFromItem()
{
	Super::RefreshFromItem();

	const UPDSettingSliderViewModel* SliderItem = Cast<UPDSettingSliderViewModel>(GetItem());
	if (!SliderItem)
	{
		return;
	}

	Slider_Value->SetMinValue(SliderItem->GetMinValue());
	Slider_Value->SetMaxValue(SliderItem->GetMaxValue());
	Slider_Value->SetStepSize(SliderItem->GetStepSize());
	Slider_Value->SetValue(SliderItem->GetValue());
	Text_Value->SetText(SliderItem->GetValueText());
}

void UPDSettingSliderRowWidget::HandleSliderValueChanged(const float Value)
{
	if (IsRefreshing())
	{
		return;
	}

	if (UPDSettingSliderViewModel* SliderItem = Cast<UPDSettingSliderViewModel>(GetItem()))
	{
		SliderItem->SetValueFromUser(Value);
	}
}
