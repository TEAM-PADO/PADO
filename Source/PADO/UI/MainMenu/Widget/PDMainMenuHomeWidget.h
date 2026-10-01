// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuScreenBase.h"
#include "PDMainMenuHomeWidget.generated.h"

class UPDButtonBase;

/**
 * 메인 메뉴 첫 화면이다. 게임 시작·게임 참가·옵션·게임 종료로 이동한다.
 * 옵션은 루트에 Options 화면이 등록된 경우에만 활성화된다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDMainMenuHomeWidget : public UPDMainMenuScreenBase
{
	GENERATED_BODY()

public:
	UPDMainMenuHomeWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void HandleStartClicked();
	void HandleJoinClicked();
	void HandleOptionsClicked();
	void HandleQuitClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Start;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Options;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Quit;
};
