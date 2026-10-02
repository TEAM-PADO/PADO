// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"
#include "PDSettingChoiceViewModel.generated.h"

/** 여러 선택지 중 하나를 고르는 옵션 줄이다. 화면 모드·해상도·그래픽 품질에 쓴다. */
UCLASS(BlueprintType)
class PADO_API UPDSettingChoiceViewModel : public UPDSettingItemViewModel
{
	GENERATED_BODY()

public:
	/** 선택지 번호를 소유자 값에서 읽고 쓰는 함수를 지정한다. */
	void SetAccessors(TFunction<int32()> InIndexGetter, TFunction<void(int32)> InIndexSetter);

	/**
	 * 숫자 목록 중 하나를 고르는 항목으로 쓴다. 선택지 문구는 InFormatter로 만든다.
	 * 지금 값이 목록에 없으면(다른 곳에서 바꾼 렌더링 해상도 등) 그 값을 선택지 끝에 덧붙여 보여 준다.
	 */
	void SetValueAccessors(const TArray<float>& InValues, TFunction<FText(float)> InFormatter,
		TFunction<float()> InGetter, TFunction<void(float)> InSetter);

	/** 선택지 문구를 바꾼다. 다음 Refresh에서 선택 번호와 함께 줄 위젯에 반영된다. */
	void SetOptions(const TArray<FText>& InOptions);

	/** 사용자가 고른 선택지를 소유자 값에 쓴다. 범위를 벗어나거나 이미 고른 선택지면 무시한다. */
	void SelectOption(int32 Index);

	const TArray<FText>& GetOptions() const { return Options; }
	int32 GetSelectedIndex() const { return SelectedIndex; }
	const FText& GetSelectedText() const { return SelectedText; }

protected:
	virtual void RefreshValue() override;

private:
	/** 숫자 선택지 항목이면 지금 값을 포함하도록 선택지를 다시 만든다. */
	void RefreshValueOptions();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	TArray<FText> Options;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	int32 SelectedIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	FText SelectedText;

	TFunction<int32()> IndexGetter;
	TFunction<void(int32)> IndexSetter;

	// 숫자 선택지 항목에서만 쓴다.
	TArray<float> Values;
	/** 화면에 보이는 선택지의 값이다. Values 뒤에 목록에 없는 지금 값이 붙을 수 있다. */
	TArray<float> ShownValues;
	TFunction<FText(float)> ValueFormatter;
	TFunction<float()> ValueGetter;
	TFunction<void(float)> ValueSetter;
};
