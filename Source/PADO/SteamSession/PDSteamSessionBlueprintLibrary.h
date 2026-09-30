#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReusableSessionTypes.h"
#include "PDSteamSessionBlueprintLibrary.generated.h"

class UReusableSteamSessionSubsystem;

/**
 * 메인 메뉴 UI가 PADO 고정 Steam 세션 설정을 중복 작성하지 않도록 하는 Blueprint 진입점이다.
 * 완료·실패 이벤트는 반환받은 UReusableSteamSessionSubsystem에서 한 번만 바인딩한다.
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

	/** 호스트 포함 정원 4명의 Steam 리슨 세션을 만들고 인게임 맵으로 이동시킨다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject"))
	static bool CreatePADOListenSession(const UObject* WorldContextObject, const FString& RoomName);

	/** PADO ProductId로만 Steam 세션을 검색한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject", ClampMin = "1", ClampMax = "100"))
	static bool FindPADOSessions(const UObject* WorldContextObject, int32 MaxResults = 50);

	/** 선택한 검색 결과에 참가한다. 방이 가득 찬 경우 플러그인이 참가 요청 전에 거절한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Steam Session", meta = (WorldContext = "WorldContextObject"))
	static bool JoinPADOSession(const UObject* WorldContextObject, const FBlueprintSessionResult& Session);
};
