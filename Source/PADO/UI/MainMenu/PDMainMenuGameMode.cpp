// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/PDMainMenuGameMode.h"

#include "PADO/UI/MainMenu/PDMainMenuPlayerController.h"

APDMainMenuGameMode::APDMainMenuGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APDMainMenuPlayerController::StaticClass();
}

bool APDMainMenuGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return false;
}
