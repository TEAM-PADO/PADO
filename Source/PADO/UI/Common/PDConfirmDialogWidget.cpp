// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Common/PDConfirmDialogWidget.h"

#include "CommonTextBlock.h"
#include "PADO/UI/Common/PDButtonBase.h"

UPDConfirmDialogWidget::UPDConfirmDialogWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;

	// 확인창이 떠 있는 동안 뒤의 화면이 입력을 받지 않게 한다.
	bIsModal = true;
}

void UPDConfirmDialogWidget::Setup(FPDConfirmDialogArgs InArgs)
{
	Args = MoveTemp(InArgs);
	bIsClosing = false;
	RemainingSeconds = static_cast<float>(Args.TimeoutSeconds);
	DisplayedSeconds = INDEX_NONE;

	if (Text_Title)
	{
		Text_Title->SetText(Args.Title);
		Text_Title->SetVisibility(Args.Title.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	Text_Body->SetText(Args.Body);
	Button_Confirm->SetButtonText(Args.ConfirmText);
	Button_Cancel->SetButtonText(Args.CancelText);

	if (Text_Countdown)
	{
		Text_Countdown->SetVisibility(Args.TimeoutSeconds > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	RefreshCountdownText();
}

void UPDConfirmDialogWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Confirm->OnClicked().AddUObject(this, &ThisClass::HandleConfirmClicked);
	Button_Cancel->OnClicked().AddUObject(this, &ThisClass::HandleCancelClicked);
}

void UPDConfirmDialogWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	RefreshCountdownText();
}

void UPDConfirmDialogWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 게임이 멈춰도 줄어들도록 타이머 대신 위젯 Tick으로 센다.
	if (Args.TimeoutSeconds <= 0 || bIsClosing || !IsActivated())
	{
		return;
	}

	RemainingSeconds -= InDeltaTime;
	if (RemainingSeconds <= 0.0f)
	{
		Close(EPDConfirmDialogResult::Cancelled);
		return;
	}

	RefreshCountdownText();
}

bool UPDConfirmDialogWidget::NativeOnHandleBackAction()
{
	Close(EPDConfirmDialogResult::Cancelled);
	return true;
}

UWidget* UPDConfirmDialogWidget::NativeGetDesiredFocusTarget() const
{
	return Button_Cancel;
}

void UPDConfirmDialogWidget::HandleConfirmClicked()
{
	Close(EPDConfirmDialogResult::Confirmed);
}

void UPDConfirmDialogWidget::HandleCancelClicked()
{
	Close(EPDConfirmDialogResult::Cancelled);
}

void UPDConfirmDialogWidget::Close(const EPDConfirmDialogResult Result)
{
	if (bIsClosing)
	{
		return;
	}

	bIsClosing = true;
	TFunction<void(EPDConfirmDialogResult)> OnClosed = MoveTemp(Args.OnClosed);
	Args.OnClosed = nullptr;

	// 창을 먼저 내린 뒤 결과를 알려, 결과 처리에서 다른 확인창을 바로 띄울 수 있게 한다.
	DeactivateWidget();
	if (OnClosed)
	{
		OnClosed(Result);
	}
}

void UPDConfirmDialogWidget::RefreshCountdownText()
{
	if (!Text_Countdown || Args.TimeoutSeconds <= 0)
	{
		return;
	}

	const int32 Seconds = FMath::Max(FMath::CeilToInt(RemainingSeconds), 0);
	if (Seconds == DisplayedSeconds)
	{
		return;
	}

	DisplayedSeconds = Seconds;
	FFormatNamedArguments FormatArgs;
	FormatArgs.Add(SecondsArgumentName, FText::AsNumber(Seconds));
	Text_Countdown->SetText(FText::Format(Args.TimeoutFormat, FormatArgs));
}
