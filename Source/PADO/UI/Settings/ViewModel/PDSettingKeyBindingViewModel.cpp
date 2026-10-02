// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/ViewModel/PDSettingKeyBindingViewModel.h"

#include "PADO/UI/Settings/PDSettingsText.h"

void UPDSettingKeyBindingViewModel::SetMapping(const FName InMappingName, TFunction<FKey()> InKeyGetter)
{
	MappingName = InMappingName;
	KeyGetter = MoveTemp(InKeyGetter);
}

void UPDSettingKeyBindingViewModel::RequestKeyCapture()
{
	if (IsEditable())
	{
		OnKeyCaptureRequested.Broadcast(this);
	}
}

void UPDSettingKeyBindingViewModel::RefreshValue()
{
	Key = KeyGetter ? KeyGetter() : EKeys::Invalid;
	UE_MVVM_SET_PROPERTY_VALUE(KeyText, Key.IsValid() ? Key.GetDisplayName(false) : PDSettingsText::Get(PDSettingsText::Key::KeyBindingsUnbound));
}
