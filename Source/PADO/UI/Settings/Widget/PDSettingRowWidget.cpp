// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingRowWidget.h"

#include "CommonTextBlock.h"
#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"
#include "PADO/UI/Settings/Widget/PDSettingRowFrameWidget.h"

void UPDSettingRowWidget::SetItem(UPDSettingItemViewModel* InItem)
{
	if (Item)
	{
		Item->OnStateChanged.Remove(StateChangedHandle);
		StateChangedHandle.Reset();
	}

	Item = InItem;
	if (Item)
	{
		StateChangedHandle = Item->OnStateChanged.AddUObject(this, &ThisClass::HandleItemStateChanged);
		HandleItemStateChanged(Item);
	}
}

void UPDSettingRowWidget::RefreshFromItem()
{
	if (Frame_Row)
	{
		Frame_Row->SetLabel(Item->GetDisplayName());
	}
	else if (Text_Label)
	{
		Text_Label->SetText(Item->GetDisplayName());
	}

	SetIsEnabled(Item->IsEditable());
	SetToolTipText(Item->GetDisabledReason());
}

void UPDSettingRowWidget::HandleItemStateChanged(UPDSettingItemViewModel* ChangedItem)
{
	if (!Item || ChangedItem != Item)
	{
		return;
	}

	TGuardValue<bool> RefreshGuard(bIsRefreshing, true);
	RefreshFromItem();
}
