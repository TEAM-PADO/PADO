// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/ViewModel/PDSettingChoiceViewModel.h"

namespace PDSettingChoiceViewModel
{
	/** 숫자 선택지와 지금 값이 같은지 볼 때의 허용 오차다. */
	constexpr float ValueTolerance = 0.001f;

	int32 FindValueIndex(const TArray<float>& InValues, const float Value)
	{
		return InValues.IndexOfByPredicate([Value](const float Candidate)
		{
			return FMath::IsNearlyEqual(Candidate, Value, ValueTolerance);
		});
	}
}

void UPDSettingChoiceViewModel::SetAccessors(TFunction<int32()> InIndexGetter, TFunction<void(int32)> InIndexSetter)
{
	IndexGetter = MoveTemp(InIndexGetter);
	IndexSetter = MoveTemp(InIndexSetter);
}

void UPDSettingChoiceViewModel::SetValueAccessors(const TArray<float>& InValues, TFunction<FText(float)> InFormatter,
	TFunction<float()> InGetter, TFunction<void(float)> InSetter)
{
	Values = InValues;
	ValueFormatter = MoveTemp(InFormatter);
	ValueGetter = MoveTemp(InGetter);
	ValueSetter = MoveTemp(InSetter);

	SetAccessors(
		[this]() { return PDSettingChoiceViewModel::FindValueIndex(ShownValues, ValueGetter()); },
		[this](const int32 Index)
		{
			if (ShownValues.IsValidIndex(Index))
			{
				ValueSetter(ShownValues[Index]);
			}
		});

	RefreshValueOptions();
}

void UPDSettingChoiceViewModel::SetOptions(const TArray<FText>& InOptions)
{
	// FText 배열은 값 비교를 지원하지 않아 바뀐 것으로 보고 바로 알린다.
	Options = InOptions;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Options);
}

void UPDSettingChoiceViewModel::SelectOption(const int32 Index)
{
	if (!IsEditable() || !IndexSetter || !Options.IsValidIndex(Index) || Index == SelectedIndex)
	{
		return;
	}

	IndexSetter(Index);
	NotifyEdited();
}

void UPDSettingChoiceViewModel::RefreshValue()
{
	RefreshValueOptions();

	const int32 NewSelectedIndex = IndexGetter ? IndexGetter() : INDEX_NONE;
	UE_MVVM_SET_PROPERTY_VALUE(SelectedIndex, NewSelectedIndex);
	UE_MVVM_SET_PROPERTY_VALUE(SelectedText, Options.IsValidIndex(NewSelectedIndex) ? Options[NewSelectedIndex] : FText::GetEmpty());
}

void UPDSettingChoiceViewModel::RefreshValueOptions()
{
	if (!ValueGetter)
	{
		return;
	}

	TArray<float> NewShownValues = Values;
	const float CurrentValue = ValueGetter();
	if (PDSettingChoiceViewModel::FindValueIndex(NewShownValues, CurrentValue) == INDEX_NONE)
	{
		NewShownValues.Add(CurrentValue);
	}

	if (NewShownValues == ShownValues)
	{
		return;
	}

	ShownValues = MoveTemp(NewShownValues);
	TArray<FText> ValueTexts;
	for (const float Value : ShownValues)
	{
		ValueTexts.Add(ValueFormatter(Value));
	}
	SetOptions(ValueTexts);
}
