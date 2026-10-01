// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDMainMenuScreenBase.h"

#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuRootWidget.h"
#include "PADO/UI/PDUILog.h"

UPDMainMenuScreenBase::UPDMainMenuScreenBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UPDMainMenuScreenBase::SetMenuRoot(UPDMainMenuRootWidget* InMenuRoot)
{
	MenuRoot = InMenuRoot;
}

void UPDMainMenuScreenBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Back)
	{
		Button_Back->OnClicked().AddUObject(this, &ThisClass::HandleBackClicked);
	}
}

void UPDMainMenuScreenBase::RequestScreen(const EPDMainMenuScreen Screen) const
{
	UPDMainMenuRootWidget* Root = MenuRoot.Get();
	if (!Root)
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s cannot open %s because it has no menu root."), *GetNameSafe(GetClass()), *UEnum::GetValueAsString(Screen));
		return;
	}

	Root->PushScreen(Screen);
}

void UPDMainMenuScreenBase::HandleBackClicked()
{
	DeactivateWidget();
}
