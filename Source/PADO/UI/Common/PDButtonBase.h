// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "PDButtonBase.generated.h"

class UCommonTextBlock;

/**
 * PADO UI 공용 버튼이다. 표시 문구는 String Table 항목을 ButtonText에 지정해 로컬라이징한다.
 * 스타일은 이 클래스를 부모로 한 WBP 한 곳에서 관리한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PADO|UI")
	void SetButtonText(const FText& InText);

protected:
	virtual void NativePreConstruct() override;

	/** 버튼에 표시할 문구다. String Table 항목을 지정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PADO|UI")
	FText ButtonText;

private:
	void RefreshButtonText();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Label;
};
