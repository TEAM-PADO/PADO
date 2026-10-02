// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDKeyCaptureWidget.h"

#include "CommonTextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "PADO/UI/Settings/Input/PDKeyCaptureInputProcessor.h"
#include "PADO/UI/Settings/PDSettingsText.h"

UPDKeyCaptureWidget::UPDKeyCaptureWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Esc는 입력 가로채기에서 취소로 처리하므로 뒤로가기 처리기로 두지 않는다.
	bIsBackHandler = false;
	bIsModal = true;
}

void UPDKeyCaptureWidget::Setup(const FText& ActionName, TFunction<void(const FKey&)> InOnKeyCaptured)
{
	OnKeyCaptured = MoveTemp(InOnKeyCaptured);

	FFormatNamedArguments Args;
	Args.Add(PDSettingsText::Arg::Action, ActionName);
	Text_Prompt->SetText(FText::Format(PDSettingsText::Get(PDSettingsText::Key::KeyBindingsCapturePrompt), Args));
}

void UPDKeyCaptureWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (FSlateApplication::IsInitialized())
	{
		InputProcessor = MakeShared<FPDKeyCaptureInputProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
	}
}

void UPDKeyCaptureWidget::NativeOnDeactivated()
{
	StopCapture();

	Super::NativeOnDeactivated();
}

void UPDKeyCaptureWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 입력 처리 중에 가로채기를 해제하지 않도록 결과는 Tick에서 처리한다.
	if (!InputProcessor.IsValid())
	{
		return;
	}

	const TOptional<FKey> CapturedKey = InputProcessor->GetCapturedKey();
	if (!CapturedKey.IsSet() && !InputProcessor->IsCancelled())
	{
		return;
	}

	TFunction<void(const FKey&)> Callback = MoveTemp(OnKeyCaptured);
	OnKeyCaptured = nullptr;
	StopCapture();
	DeactivateWidget();

	if (CapturedKey.IsSet() && Callback)
	{
		Callback(CapturedKey.GetValue());
	}
}

void UPDKeyCaptureWidget::StopCapture()
{
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}

	InputProcessor.Reset();
}
