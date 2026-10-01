#include "PADO/Core/PDGameMode.h"

#include "PADO/Core/PDGameState.h"

APDGameMode::APDGameMode()
{
	GameStateClass = APDGameState::StaticClass();
}
