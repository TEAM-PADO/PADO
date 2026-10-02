#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReusableSessionTypes.h"
#include "PDSteamSessionBlueprintLibrary.generated.h"

class UReusableSteamSessionSubsystem;

/**
 * 기존 메뉴 Blueprint와의 호환을 위한 PADO Steam 세션 Wrapper입니다.
 * 새 UI는 UPDRoomSessionFlowSubsystem을 유일한 게임 흐름 진입점으로 사용해야 합니다.
 */
UCLASS()
class PADO_API UPDSteamSessionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 현재 GameInstance가 소유한 세션 Subsystem을 반환한다.
	 * 메뉴 UI는 반환값의 완료 이벤트를 한 번만 바인딩해 생성·검색·참가 결과를 처리한다.
	 */
	UFUNCTION(BlueprintPure, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject"))
	static UReusableSteamSessionSubsystem* GetSteamSessionSubsystem(const UObject* WorldContextObject);

	/** 호스트를 포함한 PADO 방의 고정 정원(4명)을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Steam Session")
	static int32 GetMaxPlayers();

	/** Steam 테스트 App ID 480에서 PADO 세션만 검색하기 위한 프로젝트 고정 식별자를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Steam Session")
	static FString GetProductId();

	/**
	 * 호환용으로 새 방 생성 요청을 RoomSessionFlowSubsystem에 전달합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject"))
	static bool CreatePADOListenSession(const UObject* WorldContextObject, const FString& RoomName);

	/** 호환용으로 공개 방 검색 요청을 RoomSessionFlowSubsystem에 전달합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject", ClampMin = "1", ClampMax = "100"))
	static bool FindPADOSessions(const UObject* WorldContextObject, int32 MaxResults = 50);

	/** 호환용으로 선택한 검색 결과 참가 요청을 RoomSessionFlowSubsystem에 전달합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject"))
	static bool JoinPADOSession(const UObject* WorldContextObject, const FBlueprintSessionResult& Session);
};
