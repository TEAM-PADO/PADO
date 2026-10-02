// Copyright PADO. All Rights Reserved.

#include "PADO/UI/HUD/Widget/PDPerformanceOverlayWidget.h"

#include "CommonTextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/UI/Settings/PDSettingsText.h"

namespace PDPerformanceOverlayWidget
{
	/** 숫자를 바꾸는 간격(초)이다. 매 프레임 바꾸면 읽기 어렵다. */
	constexpr float UpdateInterval = 0.5f;

	FText FormatValue(const TCHAR* FormatKey, const int32 Value)
	{
		FFormatNamedArguments Args;
		Args.Add(PDSettingsText::Arg::Value, FText::AsNumber(Value, &FNumberFormattingOptions::DefaultNoGrouping()));
		return FText::Format(PDSettingsText::Get(FormatKey), Args);
	}
}

void UPDPerformanceOverlayWidget::RefreshFromSettings()
{
	const UPDGameUserSettings* Settings = UPDGameUserSettings::Get();
	const EPDPerformanceInfo PerformanceInfo = Settings ? Settings->GetPerformanceInfo() : EPDPerformanceInfo::Off;

	SetVisibility(PerformanceInfo == EPDPerformanceInfo::Off ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);

	bShowPing = PerformanceInfo == EPDPerformanceInfo::FpsAndPing && IsConnectedToServer();
	if (Text_Ping)
	{
		Text_Ping->SetVisibility(bShowPing ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	FrameCount = 0;
	ElapsedTime = 0.0f;
}

void UPDPerformanceOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 첫 평균이 나올 때까지 디자이너 예시 문구가 보이지 않게 비운다.
	Text_Fps->SetText(FText::GetEmpty());
	if (Text_Ping)
	{
		Text_Ping->SetText(FText::GetEmpty());
	}

	RefreshFromSettings();
}

void UPDPerformanceOverlayWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	++FrameCount;
	ElapsedTime += InDeltaTime;
	if (ElapsedTime < PDPerformanceOverlayWidget::UpdateInterval)
	{
		return;
	}

	UpdateTexts(FrameCount / ElapsedTime);
	FrameCount = 0;
	ElapsedTime = 0.0f;
}

void UPDPerformanceOverlayWidget::UpdateTexts(const float AverageFps)
{
	using namespace PDSettingsText::Key;

	Text_Fps->SetText(PDPerformanceOverlayWidget::FormatValue(PerformanceInfoFpsValue, FMath::RoundToInt(AverageFps)));

	if (Text_Ping && bShowPing)
	{
		const APlayerState* PlayerState = GetOwningPlayerState();
		const int32 PingMilliseconds = PlayerState ? FMath::RoundToInt(PlayerState->GetPingInMilliseconds()) : 0;
		Text_Ping->SetText(PDPerformanceOverlayWidget::FormatValue(PerformanceInfoPingValue, PingMilliseconds));
	}
}

bool UPDPerformanceOverlayWidget::IsConnectedToServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() == NM_Client;
}
