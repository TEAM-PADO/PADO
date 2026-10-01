// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Common/PDActivatableWidget.h"
#include "PADO/UI/MainMenu/Enum/PDMainMenuScreen.h"
#include "PDMainMenuScreenBase.generated.h"

class UPDButtonBase;
class UPDMainMenuRootWidget;

/**
 * 메인 메뉴 스택에 올라가는 화면의 공용 부모다.
 * 뒤로가기(Esc·Backspace·Button_Back)로 자신을 닫아 이전 화면으로 돌아간다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDMainMenuScreenBase : public UPDActivatableWidget
{
	GENERATED_BODY()

public:
	UPDMainMenuScreenBase(const FObjectInitializer& ObjectInitializer);

	/** 화면 전환을 요청할 루트 위젯을 지정한다. 스택에 올릴 때 루트가 호출한다. */
	void SetMenuRoot(UPDMainMenuRootWidget* InMenuRoot);

protected:
	virtual void NativeOnInitialized() override;

	/** 루트 위젯에 다른 화면을 올리도록 요청한다. */
	void RequestScreen(EPDMainMenuScreen Screen) const;

	UPDMainMenuRootWidget* GetMenuRoot() const { return MenuRoot.Get(); }

private:
	void HandleBackClicked();

	/** 선택 요소다. 없으면 Esc·Backspace로만 돌아간다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPDButtonBase> Button_Back;

	TWeakObjectPtr<UPDMainMenuRootWidget> MenuRoot;
};
