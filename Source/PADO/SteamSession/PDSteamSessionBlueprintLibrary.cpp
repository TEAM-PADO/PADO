#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "PADO/PADO.h"
#include "PADO/SteamSession/PDRoomSessionFlowSubsystem.h"
#include "PADO/SteamSession/PDSteamSessionSettings.h"
#include "ReusableSteamSessionSubsystem.h"

namespace
{
	UGameInstance* GetGameInstanceFromWorldContext(const UObject* WorldContextObject)
	{
		const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		return World ? World->GetGameInstance() : nullptr;
	}

	UPDRoomSessionFlowSubsystem* GetRoomSessionFlowSubsystem(const UObject* WorldContextObject)
	{
		UGameInstance* GameInstance = GetGameInstanceFromWorldContext(WorldContextObject);
		return GameInstance ? GameInstance->GetSubsystem<UPDRoomSessionFlowSubsystem>() : nullptr;
	}
}

UReusableSteamSessionSubsystem* UPDSteamSessionBlueprintLibrary::GetSteamSessionSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = GetGameInstanceFromWorldContext(WorldContextObject);
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
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the world context is unavailable."));
		return false;
	}

	UPDRoomSessionFlowSubsystem* FlowSubsystem = GetRoomSessionFlowSubsystem(WorldContextObject);
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestCreateRoom(RoomName);
}

bool UPDSteamSessionBlueprintLibrary::CreatePADOListenSessionWithAccessSettings(const UObject* WorldContextObject, const FString& RoomName, const FPDRoomAccessSettings& AccessSettings)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the world context is unavailable."));
		return false;
	}

	UPDRoomSessionFlowSubsystem* FlowSubsystem = GetRoomSessionFlowSubsystem(WorldContextObject);
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestCreateRoomWithAccessSettings(RoomName, AccessSettings);
}

bool UPDSteamSessionBlueprintLibrary::FindPADOSessions(const UObject* WorldContextObject, const int32 MaxResults)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot find Steam sessions because the world context is unavailable."));
		return false;
	}

	UPDRoomSessionFlowSubsystem* FlowSubsystem = GetRoomSessionFlowSubsystem(WorldContextObject);
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot find Steam sessions because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestFindRooms(MaxResults);
}

bool UPDSteamSessionBlueprintLibrary::JoinPADOSession(const UObject* WorldContextObject, const FBlueprintSessionResult& Session)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a Steam session because the world context is unavailable."));
		return false;
	}

	UPDRoomSessionFlowSubsystem* FlowSubsystem = GetRoomSessionFlowSubsystem(WorldContextObject);
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a Steam session because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestJoinRoom(Session);
}

bool UPDSteamSessionBlueprintLibrary::JoinPADOSessionWithPassword(const UObject* WorldContextObject, const FBlueprintSessionResult& Session, const FString& Password)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a Steam session because the world context is unavailable."));
		return false;
	}

	UPDRoomSessionFlowSubsystem* FlowSubsystem = GetRoomSessionFlowSubsystem(WorldContextObject);
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a Steam session because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestJoinRoomWithPassword(Session, Password);
}
