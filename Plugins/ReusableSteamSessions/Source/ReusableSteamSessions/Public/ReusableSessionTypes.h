#pragma once

#include "CoreMinimal.h"
#include "FindSessionsCallbackProxy.h"
#include "ReusableSessionTypes.generated.h"

/** Settings advertised with a listen-server session. ProductId prevents AppId 480 test sessions from mixing. */
USTRUCT(BlueprintType)
struct FReusableSessionSettings
{
	GENERATED_BODY()

	/** A stable per-game identifier used to filter search results. Do not leave this empty for Steam AppId 480. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session", meta = (ClampMin = "1"))
	FString ProductId = TEXT("MyGame");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	FString RoomName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxPublicConnections = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bIsLANMatch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bFriendsOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bAllowJoinInProgress = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	bool bUseLobbiesIfAvailable = true;

};

USTRUCT(BlueprintType)
struct FReusableSessionEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString RoomName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingInMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bFriendsOnly = false;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FBlueprintSessionResult SessionResult;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReusableSessionOperationComplete, bool, bSuccess, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReusableSessionJoinComplete, bool, bSuccess, const FString&, ConnectString);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReusableSessionSearchComplete, bool, bSuccess, const TArray<FReusableSessionEntry>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReusableSessionInviteAccepted, const FBlueprintSessionResult&, Session);
