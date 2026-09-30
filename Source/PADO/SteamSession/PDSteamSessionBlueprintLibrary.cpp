#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "PADO/PADO.h"
#include "PADO/SteamSession/PDSteamSessionSettings.h"
#include "ReusableSteamSessionSubsystem.h"

UReusableSteamSessionSubsystem* UPDSteamSessionBlueprintLibrary::GetSteamSessionSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UReusableSteamSessionSubsystem>() : nullptr;
}

int32 UPDSteamSessionBlueprintLibrary::GetMaxPlayers()
{
	return GetDefault<UPDSteamSessionSettings>()->GetMaxPlayers();
}

FString UPDSteamSessionBlueprintLibrary::GetProductId()
{
	return GetDefault<UPDSteamSessionSettings>()->ProductId.TrimStartAndEnd();
}

bool UPDSteamSessionBlueprintLibrary::CreatePADOListenSession(const UObject* WorldContextObject, const FString& RoomName)
{
	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem(WorldContextObject);
	const UPDSteamSessionSettings* Settings = GetDefault<UPDSteamSessionSettings>();
	if (!SessionSubsystem || !Settings)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the session subsystem or settings are unavailable."));
		return false;
	}

	const FString ProductId = Settings->ProductId.TrimStartAndEnd();
	const FString MapPath = Settings->ListenServerMap.GetLongPackageName();
	if (ProductId.IsEmpty() || MapPath.IsEmpty())
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because ProductId or ListenServerMap is empty."));
		return false;
	}

	FReusableSessionSettings SessionSettings;
	SessionSettings.ProductId = ProductId;
	SessionSettings.RoomName = RoomName;
	SessionSettings.MaxPublicConnections = Settings->GetMaxPlayers();
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bFriendsOnly = false;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.ListenServerMap = MapPath;
	SessionSubsystem->CreateListenSession(SessionSettings);
	return true;
}

bool UPDSteamSessionBlueprintLibrary::FindPADOSessions(const UObject* WorldContextObject, const int32 MaxResults)
{
	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem(WorldContextObject);
	const FString ProductId = GetProductId();
	if (!SessionSubsystem || ProductId.IsEmpty())
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot find Steam sessions because the session subsystem or ProductId is unavailable."));
		return false;
	}

	SessionSubsystem->FindSessions(ProductId, FMath::Clamp(MaxResults, 1, 100), false);
	return true;
}

bool UPDSteamSessionBlueprintLibrary::JoinPADOSession(const UObject* WorldContextObject, const FBlueprintSessionResult& Session)
{
	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem(WorldContextObject);
	if (!SessionSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a Steam session because the session subsystem is unavailable."));
		return false;
	}

	SessionSubsystem->JoinSession(Session);
	return true;
}
