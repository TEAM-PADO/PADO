// Copyright PADO. All Rights Reserved.

#include "PADO/UI/HUD/PDPerformanceOverlaySubsystem.h"

#include "GameFramework/PlayerController.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/UI/HUD/PDHUDSettings.h"
#include "PADO/UI/HUD/Widget/PDPerformanceOverlayWidget.h"
#include "PADO/UI/PDUILog.h"

void UPDPerformanceOverlaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UPDGameUserSettings* Settings = UPDGameUserSettings::Get())
	{
		SettingsAppliedHandle = Settings->OnSettingsApplied.AddUObject(this, &ThisClass::HandleSettingsApplied);
	}
}

void UPDPerformanceOverlaySubsystem::Deinitialize()
{
	if (UPDGameUserSettings* Settings = UPDGameUserSettings::Get())
	{
		Settings->OnSettingsApplied.Remove(SettingsAppliedHandle);
	}
	SettingsAppliedHandle.Reset();

	if (OverlayWidget)
	{
		OverlayWidget->RemoveFromParent();
		OverlayWidget = nullptr;
	}

	Super::Deinitialize();
}

void UPDPerformanceOverlaySubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (OverlayWidget)
	{
		OverlayWidget->RemoveFromParent();
		OverlayWidget = nullptr;
	}

	if (!NewPlayerController || !NewPlayerController->IsLocalController())
	{
		return;
	}

	const UPDHUDSettings* HUDSettings = GetDefault<UPDHUDSettings>();
	const TSubclassOf<UPDPerformanceOverlayWidget> OverlayClass = HUDSettings->PerformanceOverlayClass.LoadSynchronous();
	if (!OverlayClass)
	{
		UE_LOG(LogPDUI, Warning, TEXT("Performance info overlay was not created. Assign Performance Overlay Class in Project Settings > PADO HUD."));
		return;
	}

	// 메뉴·HUD와 같은 뷰포트 층에 올려야 Z 순서로 그 위에 그려진다. 플레이어 화면 층은 뷰포트 층보다 아래다.
	OverlayWidget = CreateWidget<UPDPerformanceOverlayWidget>(NewPlayerController, OverlayClass);
	if (OverlayWidget)
	{
		OverlayWidget->AddToViewport(HUDSettings->PerformanceOverlayZOrder);
	}
}

void UPDPerformanceOverlaySubsystem::HandleSettingsApplied()
{
	if (OverlayWidget)
	{
		OverlayWidget->RefreshFromSettings();
	}
}
