// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/ViewModel/PDSettingToggleViewModel.h"

void UPDSettingToggleViewModel::SetAccessors(TFunction<bool()> InValueGetter, TFunction<void(bool)> InValueSetter)
{
	ValueGetter = MoveTemp(InValueGetter);
	ValueSetter = MoveTemp(InValueSetter);
}

void UPDSettingToggleViewModel::SetValueTexts(const FText& InOnText, const FText& InOffText)
{
	OnText = InOnText;
	OffText = InOffText;
}

void UPDSettingToggleViewModel::SetIsOnFromUser(const bool bInIsOn)
{
	if (!IsEditable() || !ValueSetter || bInIsOn == bIsOn)
	{
		return;
	}

	ValueSetter(bInIsOn);
	NotifyEdited();
}

void UPDSettingToggleViewModel::RefreshValue()
{
	const bool bNewIsOn = ValueGetter && ValueGetter();
	UE_MVVM_SET_PROPERTY_VALUE(bIsOn, bNewIsOn);
	UE_MVVM_SET_PROPERTY_VALUE(ValueText, bNewIsOn ? OnText : OffText);
}
