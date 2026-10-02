// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Common/PDActivatableWidget.h"
#include "PADO/UI/Common/Struct/PDConfirmDialogArgs.h"
#include "PDConfirmDialogWidget.generated.h"

class UCommonTextBlock;
class UPDButtonBase;

/**
 * PADO UI 공용 확인창이다. 띄운 화면의 Activatable Widget Stack(대화상자용)에 올려 쓴다.
 * 뒤로가기(Esc)와 시간 초과는 취소로 닫는다. 위험한 선택을 막도록 처음 초점은 취소 버튼에 둔다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDConfirmDialogWidget : public UPDActivatableWidget
{
	GENERATED_BODY()

public:
	UPDConfirmDialogWidget(const FObjectInitializer& ObjectInitializer);

	/** 내용을 채운다. 스택에 올릴 때 한 번 호출한다. */
	void Setup(FPDConfirmDialogArgs InArgs);

	/** 남은 시간 문구 형식에서 남은 초를 넣는 인자 이름이다. */
	static constexpr const TCHAR* SecondsArgumentName = TEXT("Seconds");

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void HandleConfirmClicked();
	void HandleCancelClicked();
	void Close(EPDConfirmDialogResult Result);
	void RefreshCountdownText();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Body;

	/** 선택 요소다. 시간 제한이 있는 확인창에서 남은 시간을 보여 준다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Countdown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Confirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Cancel;

	FPDConfirmDialogArgs Args;
	float RemainingSeconds = 0.0f;
	int32 DisplayedSeconds = INDEX_NONE;
	bool bIsClosing = false;
};
