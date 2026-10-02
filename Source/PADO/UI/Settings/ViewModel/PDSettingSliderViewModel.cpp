// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/ViewModel/PDSettingSliderViewModel.h"

void UPDSettingSliderViewModel::SetRange(const float InMinValue, const float InMaxValue, const float InStepSize)
{
	MinValue = FMath::Min(InMinValue, InMaxValue);
	MaxValue = FMath::Max(InMinValue, InMaxValue);
	StepSize = FMath::Max(InStepSize, 0.0f);
}

void UPDSettingSliderViewModel::SetAccessors(TFunction<float()> InValueGetter, TFunction<void(float)> InValueSetter, TFunction<FText(float)> InValueFormatter)
{
	ValueGetter = MoveTemp(InValueGetter);
	ValueSetter = MoveTemp(InValueSetter);
	ValueFormatter = MoveTemp(InValueFormatter);
}

void UPDSettingSliderViewModel::SetValueFromUser(const float InValue)
{
	if (!IsEditable() || !ValueSetter)
	{
		return;
	}

	const float NewValue = SnapValue(InValue);
	if (FMath::IsNearlyEqual(NewValue, Value))
	{
		return;
	}

	ValueSetter(NewValue);
	NotifyEdited();
}

float UPDSettingSliderViewModel::SnapValue(const float InValue) const
{
	float SnappedValue = FMath::Clamp(InValue, MinValue, MaxValue);
	if (StepSize > 0.0f)
	{
		SnappedValue = MinValue + FMath::RoundToFloat((SnappedValue - MinValue) / StepSize) * StepSize;
	}

	return FMath::Clamp(SnappedValue, MinValue, MaxValue);
}

void UPDSettingSliderViewModel::RefreshValue()
{
	const float NewValue = ValueGetter ? SnapValue(ValueGetter()) : MinValue;
	UE_MVVM_SET_PROPERTY_VALUE(Value, NewValue);
	UE_MVVM_SET_PROPERTY_VALUE(ValueText, ValueFormatter ? ValueFormatter(NewValue) : FText::AsNumber(NewValue));
}
