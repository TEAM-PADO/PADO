#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Package.h"

namespace PDTestWorldUtils
{
	/**
	 * 자동화 테스트용 Game World다. 액터 초기화를 거치지 않으므로 스폰해도
	 * InitializeComponent와 PostInitializeComponents가 돌지 않고, 인터페이스
	 * 이벤트(ProcessEvent)도 실행되지 않는다.
	 */
	inline UWorld* CreateTestWorld(FWorldContext*& OutWorldContext)
	{
		const FName WorldName = MakeUniqueObjectName(
			nullptr,
			UWorld::StaticClass(),
			TEXT("PDTestWorld"),
			EUniqueObjectNameOptions::GloballyUnique);
		UWorld* World = UWorld::CreateWorld(
			EWorldType::Game,
			false,
			WorldName,
			GetTransientPackage());
		OutWorldContext = World
			? &GEngine->CreateNewWorldContext(EWorldType::Game)
			: nullptr;
		if (OutWorldContext)
		{
			OutWorldContext->SetCurrentWorld(World);
		}
		return World;
	}

	/**
	 * 액터 초기화까지 마친 World다. 이후 스폰은 실제 게임처럼
	 * InitializeComponent와 PostInitializeComponents를 거치고, 파괴하면 EndPlay가
	 * 돈다. Epic의 GAS 테스트와 같은 준비 방식이다.
	 */
	inline UWorld* CreateInitializedTestWorld(FWorldContext*& OutWorldContext)
	{
		UWorld* World = CreateTestWorld(OutWorldContext);
		if (World)
		{
			World->InitializeActorsForPlay(FURL());
		}
		return World;
	}

	inline void DestroyTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}
}

#endif
