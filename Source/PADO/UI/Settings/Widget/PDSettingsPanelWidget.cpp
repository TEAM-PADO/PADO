// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingsPanelWidget.h"

#include "Components/PanelWidget.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "PADO/UI/PDUILog.h"
#include "PADO/UI/Settings/PDSettingsDesignerPreview.h"
#include "PADO/UI/Settings/ViewModel/PDSettingChoiceViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingHeaderViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingKeyBindingViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingSliderViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingToggleViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingsViewModel.h"
#include "PADO/UI/Settings/Widget/PDSettingChoiceRowWidget.h"
#include "PADO/UI/Settings/Widget/PDSettingKeyBindingRowWidget.h"
#include "PADO/UI/Settings/Widget/PDSettingSliderRowWidget.h"
#include "PADO/UI/Settings/Widget/PDSettingToggleRowWidget.h"

void UPDSettingsPanelWidget::SetItems(const TArray<UPDSettingItemViewModel*>& Items)
{
	bHasAssignedItems = true;
	BuildRows(Items);
}

void UPDSettingsPanelWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

#if WITH_EDITOR
	// 이 WBP만 디자이너에서 열었을 때 기본값 줄을 그린다. 옵션 창 안에서는 옵션 창이 항목을 넘긴다.
	if (IsDesignTime() && !bHasAssignedItems)
	{
		if (!DesignerPreviewViewModel)
		{
			DesignerPreviewViewModel = PDSettingsDesignerPreview::CreateViewModel(this);
		}

		BuildRows(DesignerPreviewViewModel->GetItems(DesignerPreviewTab));
	}
#endif
}

void UPDSettingsPanelWidget::BuildRows(const TArray<UPDSettingItemViewModel*>& Items)
{
	Panel_Rows->ClearChildren();
	Rows.Reset();

	for (UPDSettingItemViewModel* Item : Items)
	{
		const TSubclassOf<UPDSettingRowWidget> RowClass = Item ? GetRowClass(*Item) : nullptr;
		if (!RowClass)
		{
			UE_LOG(LogPDUI, Warning, TEXT("%s has no row class for setting %s. Assign the row classes in the widget defaults."),
				*GetNameSafe(GetClass()), Item ? *Item->GetItemId().ToString() : TEXT("None"));
			continue;
		}

		UPDSettingRowWidget* Row = CreateWidget<UPDSettingRowWidget>(this, RowClass);
		Row->SetItem(Item);
		Rows.Add(Row);

		UPanelSlot* RowSlot = Panel_Rows->AddChild(Row);
		if (UScrollBoxSlot* ScrollBoxSlot = Cast<UScrollBoxSlot>(RowSlot))
		{
			ScrollBoxSlot->SetPadding(RowPadding);
		}
		else if (UVerticalBoxSlot* VerticalBoxSlot = Cast<UVerticalBoxSlot>(RowSlot))
		{
			VerticalBoxSlot->SetPadding(RowPadding);
		}
	}
}

UWidget* UPDSettingsPanelWidget::GetFirstFocusTarget() const
{
	for (const UPDSettingRowWidget* Row : Rows)
	{
		if (Row->GetIsEnabled())
		{
			if (UWidget* FocusTarget = Row->GetFocusTarget())
			{
				return FocusTarget;
			}
		}
	}

	return nullptr;
}

TSubclassOf<UPDSettingRowWidget> UPDSettingsPanelWidget::GetRowClass(const UPDSettingItemViewModel& Item) const
{
	if (Item.IsA<UPDSettingChoiceViewModel>())
	{
		return ChoiceRowClass;
	}

	if (Item.IsA<UPDSettingSliderViewModel>())
	{
		return SliderRowClass;
	}

	if (Item.IsA<UPDSettingToggleViewModel>())
	{
		return ToggleRowClass;
	}

	if (Item.IsA<UPDSettingKeyBindingViewModel>())
	{
		return KeyBindingRowClass;
	}

	if (Item.IsA<UPDSettingHeaderViewModel>())
	{
		return HeaderRowClass;
	}

	return nullptr;
}
