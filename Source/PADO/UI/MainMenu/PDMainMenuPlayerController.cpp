// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/PDMainMenuPlayerController.h"

#include "PADO/UI/MainMenu/Widget/PDMainMenuRootWidget.h"
#include "PADO/UI/PDUILog.h"

void APDMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!MainMenuRootWidgetClass)
	{
		UE_LOG(LogPDUI, Error, TEXT("%s has no MainMenuRootWidgetClass. Assign WBP_MainMenuRoot in the Blueprint defaults."), *GetNameSafe(GetClass()));
		return;
	}

	MainMenuRootWidget = CreateWidget<UPDMainMenuRootWidget>(this, MainMenuRootWidgetClass);
	if (!MainMenuRootWidget)
	{
		UE_LOG(LogPDUI, Error, TEXT("Failed to create main menu root widget %s."), *GetNameSafe(MainMenuRootWidgetClass));
		return;
	}

	MainMenuRootWidget->AddToViewport();
	SetShowMouseCursor(true);
}

void APDMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MainMenuRootWidget)
	{
		MainMenuRootWidget->RemoveFromParent();
		MainMenuRootWidget = nullptr;
	}

	// 메뉴 화면이 CommonUI로 바꾼 마우스 캡처는 맵을 옮겨도 유지되는 게임 뷰포트에 남는다.
	// 게임 맵에서 마우스 시점이 동작하도록 메뉴를 떠날 때 게임 입력으로 되돌린다.
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}

	Super::EndPlay(EndPlayReason);
}
