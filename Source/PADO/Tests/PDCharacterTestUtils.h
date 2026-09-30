#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Character/PDPlayerState.h"

namespace PDCharacterTestUtils
{
	/**
	 * 플레이어의 ASC를 가진 PlayerState를 만든다.
	 *
	 * 액터 초기화를 거치지 않은 World에서는 스폰해도 InitializeComponent가
	 * 돌지 않는다. 그러면 ASC가 PlayerState의 Attribute Set을 수집하지 못하므로,
	 * 실제 스폰과 같은 상태가 되도록 한 번 부른다.
	 */
	inline APDPlayerState* SpawnPlayerState(UWorld* World)
	{
		APDPlayerState* PlayerState =
			World ? World->SpawnActor<APDPlayerState>() : nullptr;
		if (PlayerState && !World->AreActorsInitialized())
		{
			PlayerState->InitializeComponents();
		}
		return PlayerState;
	}

	/**
	 * PlayerState를 연결한 플레이어 캐릭터를 만든다.
	 *
	 * 실제 게임처럼 ASC는 PlayerState가 소유하고 캐릭터는 아바타가 된다.
	 * 컨트롤러는 붙이지 않는다. 빙의 경계를 거치는 연결은
	 * PADO.Character.AbilitySystem 테스트가 따로 검증한다.
	 */
	inline APDPlayerCharacter* SpawnPlayerCharacter(
		UWorld* World,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		if (!World)
		{
			return nullptr;
		}

		APDPlayerCharacter* Character =
			World->SpawnActor<APDPlayerCharacter>(Location, Rotation);
		APDPlayerState* PlayerState = SpawnPlayerState(World);
		if (!Character || !PlayerState)
		{
			return nullptr;
		}

		Character->SetPlayerState(PlayerState);
		Character->InitializeAbilitySystem(
			PlayerState->GetPDAbilitySystemComponent(),
			PlayerState);
		return Character;
	}
}

#endif
