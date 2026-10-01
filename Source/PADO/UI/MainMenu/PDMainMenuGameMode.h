// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PDMainMenuGameMode.generated.h"

/**
 * 메인 메뉴 레벨 전용 GameMode다.
 * 플레이어 캐릭터를 생성하지 않고 메뉴용 PlayerController만 둔다.
 * L_MainMenu의 World Settings에서 GameMode Override로 지정하며, 게임 레벨의 GameMode에는 영향을 주지 않는다.
 */
UCLASS(Blueprintable)
class PADO_API APDMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDMainMenuGameMode(const FObjectInitializer& ObjectInitializer);

protected:
	/** 메뉴에서는 Pawn을 만들지 않는다. */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
};
