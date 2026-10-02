// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"
#include "PDSettingSliderViewModel.generated.h"

/** 범위 안의 수치를 고르는 옵션 줄이다. 값은 화면에 보이는 단위(밝기 0~100 등)로 다룬다. */
UCLASS(BlueprintType)
class PADO_API UPDSettingSliderViewModel : public UPDSettingItemViewModel
{
	GENERATED_BODY()

public:
	/** 값 범위와 눈금 간격을 지정한다. 눈금이 0이면 범위 안 아무 값이나 쓴다. */
	void SetRange(float InMinValue, float InMaxValue, float InStepSize);

	/** 값을 소유자 값에서 읽고 쓰는 함수와 값 문구를 만드는 함수를 지정한다. */
	void SetAccessors(TFunction<float()> InValueGetter, TFunction<void(float)> InValueSetter, TFunction<FText(float)> InValueFormatter);

	/** 사용자가 움직인 값을 범위와 눈금에 맞춰 소유자 값에 쓴다. */
	void SetValueFromUser(float InValue);

	/** 범위와 눈금에 맞춘 값을 반환한다. */
	float SnapValue(float InValue) const;

	float GetValue() const { return Value; }
	float GetMinValue() const { return MinValue; }
	float GetMaxValue() const { return MaxValue; }
	float GetStepSize() const { return StepSize; }
	const FText& GetValueText() const { return ValueText; }

protected:
	virtual void RefreshValue() override;

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	float Value = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	FText ValueText;

	float MinValue = 0.0f;
	float MaxValue = 1.0f;
	float StepSize = 0.0f;
	TFunction<float()> ValueGetter;
	TFunction<void(float)> ValueSetter;
	TFunction<FText(float)> ValueFormatter;
};
