// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingChoiceRowWidget.h"

#include "Components/ComboBoxString.h"
#include "PADO/UI/Settings/ViewModel/PDSettingChoiceViewModel.h"

UWidget* UPDSettingChoiceRowWidget::GetFocusTarget() const
{
	return ComboBox_Value;
}

void UPDSettingChoiceRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ComboBox_Value->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleSelectionChanged);
}

void UPDSettingChoiceRowWidget::RefreshFromItem()
{
	Super::RefreshFromItem();

	const UPDSettingChoiceViewModel* ChoiceItem = Cast<UPDSettingChoiceViewModel>(GetItem());
	if (!ChoiceItem)
	{
		return;
	}

	TArray<FString> Options;
	for (const FText& Option : ChoiceItem->GetOptions())
	{
		Options.Add(Option.ToString());
	}

	if (Options != BuiltOptions)
	{
		BuiltOptions = MoveTemp(Options);
		ComboBox_Value->ClearOptions();
		for (const FString& Option : BuiltOptions)
		{
			ComboBox_Value->AddOption(Option);
		}
	}

	const int32 SelectedIndex = ChoiceItem->GetSelectedIndex();
	if (BuiltOptions.IsValidIndex(SelectedIndex))
	{
		ComboBox_Value->SetSelectedIndex(SelectedIndex);
	}
	else
	{
		ComboBox_Value->ClearSelection();
	}
}

void UPDSettingChoiceRowWidget::HandleSelectionChanged(FString SelectedItem, const ESelectInfo::Type SelectionType)
{
	// 코드로 바꾼 선택(Direct)은 항목 값을 표시한 결과이므로 다시 쓰지 않는다.
	if (IsRefreshing() || SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	if (UPDSettingChoiceViewModel* ChoiceItem = Cast<UPDSettingChoiceViewModel>(GetItem()))
	{
		ChoiceItem->SelectOption(ComboBox_Value->GetSelectedIndex());
	}
}
