// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"

void UPDSettingItemViewModel::InitializeItem(const FName InItemId, const EPDSettingsTab InTab, const FText& InDisplayName)
{
	ItemId = InItemId;
	Tab = InTab;
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InDisplayName);
}

void UPDSettingItemViewModel::SetEditableCondition(TFunction<bool()> InEditableGetter, const FText& InDisabledReason)
{
	EditableGetter = MoveTemp(InEditableGetter);
	ConditionDisabledReason = InDisabledReason;
}

void UPDSettingItemViewModel::Refresh()
{
	const bool bNewIsEditable = !EditableGetter || EditableGetter();
	UE_MVVM_SET_PROPERTY_VALUE(bIsEditable, bNewIsEditable);
	UE_MVVM_SET_PROPERTY_VALUE(DisabledReason, bNewIsEditable ? FText::GetEmpty() : ConditionDisabledReason);

	RefreshValue();
	OnStateChanged.Broadcast(this);
}

void UPDSettingItemViewModel::NotifyEdited()
{
	OnEdited.Broadcast(this);
}
