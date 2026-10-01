// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDMainMenuScreen.generated.h"

/** 메인 메뉴 화면 스택에 올릴 수 있는 화면 종류다. */
UENUM(BlueprintType)
enum class EPDMainMenuScreen : uint8
{
	Home		UMETA(DisplayName = "Home"),
	GameStart	UMETA(DisplayName = "Game Start"),
	NewGame		UMETA(DisplayName = "New Game"),
	JoinGame	UMETA(DisplayName = "Join Game"),
	Options		UMETA(DisplayName = "Options")
};
