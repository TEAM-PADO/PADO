// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDMainMenuRootWidget.h"

#include "PADO/UI/Common/PDViewModelUtils.h"
#include "PADO/UI/MainMenu/ViewModel/PDMainMenuViewModel.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuScreenBase.h"
#include "PADO/UI/PDUILog.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UPDMainMenuRootWidget::PushScreen(const EPDMainMenuScreen Screen)
{
	const TSubclassOf<UPDActivatableWidget>* ScreenClass = ScreenClasses.Find(Screen);
	if (!ScreenClass || !*ScreenClass)
	{
		UE_LOG(LogPDUI, Warning, TEXT("Main menu screen %s is not registered in ScreenClasses."), *UEnum::GetValueAsString(Screen));
		return;
	}

	Stack_Screens->AddWidget<UPDActivatableWidget>(*ScreenClass, [this](UPDActivatableWidget& ScreenWidget)
	{
		// 옵션 창처럼 메뉴 루트를 모르는 화면은 스스로 닫히기만 한다.
		if (UPDMainMenuScreenBase* MenuScreen = Cast<UPDMainMenuScreenBase>(&ScreenWidget))
		{
			MenuScreen->SetMenuRoot(this);
		}
	});
}

bool UPDMainMenuRootWidget::HasScreen(const EPDMainMenuScreen Screen) const
{
	const TSubclassOf<UPDActivatableWidget>* ScreenClass = ScreenClasses.Find(Screen);
	return ScreenClass && *ScreenClass;
}

void UPDMainMenuRootWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	MainMenuViewModel = NewObject<UPDMainMenuViewModel>(this);
	PDViewModelUtils::SetViewModel(this, MainMenuViewModel);
}

void UPDMainMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MainMenuViewModel->Initialize(GetGameInstance());
	if (!Stack_Screens->GetActiveWidget())
	{
		PushScreen(EPDMainMenuScreen::Home);
	}
}

void UPDMainMenuRootWidget::NativeDestruct()
{
	MainMenuViewModel->Deinitialize();

	Super::NativeDestruct();
}
