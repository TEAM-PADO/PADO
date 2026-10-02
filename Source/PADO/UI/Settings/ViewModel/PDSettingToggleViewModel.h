// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"
#include "PDSettingToggleViewModel.generated.h"

/** 켜기/끄기를 고르는 옵션 줄이다. 수직 동기화·마우스 반전에 쓴다. */
UCLASS(BlueprintType)
class PADO_API UPDSettingToggleViewModel : public UPDSettingItemViewModel
{
	GENERATED_BODY()

public:
	/** 값을 소유자 값에서 읽고 쓰는 함수를 지정한다. */
	void SetAccessors(TFunction<bool()> InValueGetter, TFunction<void(bool)> InValueSetter);

	/** 켜짐·꺼짐 상태 옆에 보여 줄 문구를 지정한다. */
	void SetValueTexts(const FText& InOnText, const FText& InOffText);

	/** 사용자가 바꾼 상태를 소유자 값에 쓴다. */
	void SetIsOnFromUser(bool bInIsOn);

	bool IsOn() const { return bIsOn; }
	const FText& GetValueText() const { return ValueText; }

protected:
	virtual void RefreshValue() override;

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	bool bIsOn = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	FText ValueText;

	TFunction<bool()> ValueGetter;
	TFunction<void(bool)> ValueSetter;
	FText OnText;
	FText OffText;
};
