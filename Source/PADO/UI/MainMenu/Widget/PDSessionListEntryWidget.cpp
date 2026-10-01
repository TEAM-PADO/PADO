// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDSessionListEntryWidget.h"

#include "Components/Widget.h"
#include "PADO/UI/Common/PDViewModelUtils.h"
#include "PADO/UI/MainMenu/ViewModel/PDSessionEntryViewModel.h"

void UPDSessionListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	PDViewModelUtils::SetViewModel(this, Cast<UPDSessionEntryViewModel>(ListItemObject));
}

void UPDSessionListEntryWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	if (Image_Selected)
	{
		Image_Selected->SetVisibility(bIsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
