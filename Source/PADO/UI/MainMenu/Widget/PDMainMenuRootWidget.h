// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PADO/UI/MainMenu/Enum/PDMainMenuScreen.h"
#include "PDMainMenuRootWidget.generated.h"

class UCommonActivatableWidgetStack;
class UPDMainMenuScreenBase;
class UPDMainMenuViewModel;

/**
 * 메인 메뉴 레벨의 최상위 위젯이다.
 * 화면 스택을 소유하고 화면 전환 요청을 받아 ScreenClasses에 등록한 WBP를 올린다.
 * 어느 화면에 있든 표시할 초대 참가 안내는 UPDMainMenuViewModel에 바인딩한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDMainMenuRootWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 지정한 화면을 스택에 올린다. 등록되지 않은 화면이면 경고를 남기고 무시한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|MainMenu")
	void PushScreen(EPDMainMenuScreen Screen);

	/** 지정한 화면의 WBP가 등록돼 있는지 확인한다. 준비되지 않은 화면의 진입 버튼을 비활성화할 때 쓴다. */
	UFUNCTION(BlueprintPure, Category = "PADO|MainMenu")
	bool HasScreen(EPDMainMenuScreen Screen) const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 화면 종류별 WBP 클래스다. WBP_MainMenuRoot의 기본값에서 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|MainMenu")
	TMap<EPDMainMenuScreen, TSubclassOf<UPDMainMenuScreenBase>> ScreenClasses;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> Stack_Screens;

	UPROPERTY(Transient)
	TObjectPtr<UPDMainMenuViewModel> MainMenuViewModel;
};
