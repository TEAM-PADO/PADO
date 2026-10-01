#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PDSteamSessionSettings.generated.h"

/**
 * Steam 전용 PADO 세션의 프로젝트 설정이다.
 * 실제 세션 생성·검색·참가 처리는 ReusableSteamSessions 플러그인에 위임한다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "PADO Steam Session"))
class PADO_API UPDSteamSessionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Steam AppId 480 환경에서 다른 게임의 세션을 검색하지 않기 위한 PADO 전용 식별자다. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Steam Session")
	FString ProductId;

	/** 방 생성 성공 후 호스트가 ?listen으로 이동할 인게임 맵이다. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Steam Session", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath ListenServerMap;

	/** PADO의 방 정원은 호스트를 포함해 항상 4명이다. */
	UFUNCTION(BlueprintPure, Category = "Steam Session")
	int32 GetMaxPlayers() const
	{
		return 4;
	}
};
