#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "PADO/PADO.h"
#include "PADO/SteamSession/PDRoomSessionFlowSubsystem.h"
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
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the world context is unavailable."));
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UPDRoomSessionFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UPDRoomSessionFlowSubsystem>() : nullptr;
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a Steam session because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestCreateRoom(RoomName);
}

bool UPDSteamSessionBlueprintLibrary::FindPADOSessions(const UObject* WorldContextObject, const int32 MaxResults)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot find Steam sessions because the world context is unavailable."));
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UPDRoomSessionFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UPDRoomSessionFlowSubsystem>() : nullptr;
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

	UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UPDRoomSessionFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UPDRoomSessionFlowSubsystem>() : nullptr;
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a Steam session because the room session flow subsystem is unavailable."));
		return false;
	}

	return FlowSubsystem->RequestJoinRoom(Session);
}
