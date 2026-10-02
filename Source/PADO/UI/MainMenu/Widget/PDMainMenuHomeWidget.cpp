// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDMainMenuHomeWidget.h"

#include "Kismet/KismetSystemLibrary.h"
#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuRootWidget.h"

UPDMainMenuHomeWidget::UPDMainMenuHomeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 첫 화면은 뒤로가기로 닫지 않는다.
	bIsBackHandler = false;
}

void UPDMainMenuHomeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Start->OnClicked().AddUObject(this, &ThisClass::HandleStartClicked);
	Button_Join->OnClicked().AddUObject(this, &ThisClass::HandleJoinClicked);
	Button_Options->OnClicked().AddUObject(this, &ThisClass::HandleOptionsClicked);
	Button_Quit->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
}

void UPDMainMenuHomeWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	const UPDMainMenuRootWidget* Root = GetMenuRoot();
	Button_Options->SetIsEnabled(Root && Root->HasScreen(EPDMainMenuScreen::Options));
}

UWidget* UPDMainMenuHomeWidget::NativeGetDesiredFocusTarget() const
{
	return Button_Start;
}

void UPDMainMenuHomeWidget::HandleStartClicked()
{
	RequestScreen(EPDMainMenuScreen::GameStart);
}

void UPDMainMenuHomeWidget::HandleJoinClicked()
{
	RequestScreen(EPDMainMenuScreen::JoinGame);
}

void UPDMainMenuHomeWidget::HandleOptionsClicked()
{
	RequestScreen(EPDMainMenuScreen::Options);
}

void UPDMainMenuHomeWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
