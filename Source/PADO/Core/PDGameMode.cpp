#include "PADO/Core/PDGameMode.h"

#include "PADO/Character/PDPlayerState.h"

APDGameMode::APDGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 플레이어의 Ability System은 PlayerState가 소유한다. 다른 클래스를 쓰면
	// 플레이어 캐릭터가 연결할 ASC가 없다.
	PlayerStateClass = APDPlayerState::StaticClass();
}
