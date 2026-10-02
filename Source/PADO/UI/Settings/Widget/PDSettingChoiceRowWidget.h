// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/Widget/PDSettingRowWidget.h"
#include "PDSettingChoiceRowWidget.generated.h"

class UComboBoxString;

/**
 * 드롭다운으로 선택지 하나를 고르는 옵션 줄이다. UPDSettingChoiceViewModel 항목을 표시한다.
 * 선택지 글꼴·색·펼친 목록 모양은 WBP의 ComboBox_Value에서 직접 정한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingChoiceRowWidget : public UPDSettingRowWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetFocusTarget() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void RefreshFromItem() override;

private:
	UFUNCTION()
	void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ComboBox_Value;

	/** 드롭다운에 넣은 선택지 문구다. 문구가 바뀌면(선택지 목록 변경, 언어 변경) 다시 채운다. */
	TArray<FString> BuiltOptions;
};
