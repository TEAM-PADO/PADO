// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuScreenBase.h"
#include "PDGameStartScreenWidget.generated.h"

class UPDButtonBase;
class UPDGameStartViewModel;

/**
 * 게임 시작 화면이다. 저장 데이터 목록과 새 게임 진입을 제공한다.
 * 저장 데이터 목록·이어하기·삭제는 저장 계약이 정해진 뒤 연결한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDGameStartScreenWidget : public UPDMainMenuScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void HandleNewGameClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_NewGame;

	UPROPERTY(Transient)
	TObjectPtr<UPDGameStartViewModel> GameStartViewModel;
};
