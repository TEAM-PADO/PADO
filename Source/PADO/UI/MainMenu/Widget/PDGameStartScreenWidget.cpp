// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDGameStartScreenWidget.h"

#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/Common/PDViewModelUtils.h"
#include "PADO/UI/MainMenu/ViewModel/PDGameStartViewModel.h"

void UPDGameStartScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	GameStartViewModel = NewObject<UPDGameStartViewModel>(this);
	PDViewModelUtils::SetViewModel(this, GameStartViewModel);

	Button_NewGame->OnClicked().AddUObject(this, &ThisClass::HandleNewGameClicked);
}

UWidget* UPDGameStartScreenWidget::NativeGetDesiredFocusTarget() const
{
	return Button_NewGame;
}

void UPDGameStartScreenWidget::HandleNewGameClicked()
{
	RequestScreen(EPDMainMenuScreen::NewGame);
}
