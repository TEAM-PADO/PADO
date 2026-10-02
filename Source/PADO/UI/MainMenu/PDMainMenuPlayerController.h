// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PDMainMenuPlayerController.generated.h"

class UPDMainMenuRootWidget;

/**
 * 메인 메뉴 레벨의 PlayerController다.
 * 로컬 플레이어에게만 메뉴 루트 위젯을 띄우고 마우스 커서를 표시한다.
 */
UCLASS(Blueprintable)
class PADO_API APDMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 화면에 띄울 메뉴 루트 WBP다. BP_PDMainMenuPlayerController에서 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|UI")
	TSubclassOf<UPDMainMenuRootWidget> MainMenuRootWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPDMainMenuRootWidget> MainMenuRootWidget;
};
