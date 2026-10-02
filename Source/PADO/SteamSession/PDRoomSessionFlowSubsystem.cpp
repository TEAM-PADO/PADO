// Copyright PADO. All Rights Reserved.

#include "PADO/SteamSession/PDRoomSessionFlowSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "PADO/Core/PDGameMode.h"
#include "PADO/PADO.h"
#include "PADO/Save/PDRoomSaveSubsystem.h"
#include "PADO/SteamSession/PDSteamSessionSettings.h"
#include "ReusableSteamSessionSubsystem.h"

void UPDRoomSessionFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UPDRoomSaveSubsystem>();
	Collection.InitializeDependency<UReusableSteamSessionSubsystem>();

	if (UPDRoomSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UPDRoomSaveSubsystem>())
	{
		SaveSubsystem->OnRoomSaveCompleted.AddUniqueDynamic(this, &ThisClass::HandleRoomSaveCompleted);
	}

	if (UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem())
	{
		SessionSubsystem->OnCreateComplete.AddUniqueDynamic(this, &ThisClass::HandleSessionCreateCompleted);
		SessionSubsystem->OnFindComplete.AddUniqueDynamic(this, &ThisClass::HandleSessionFindCompleted);
		SessionSubsystem->OnJoinComplete.AddUniqueDynamic(this, &ThisClass::HandleSessionJoinCompleted);
		SessionSubsystem->OnInviteAccepted.AddUniqueDynamic(this, &ThisClass::HandleSteamInviteAccepted);
		SessionSubsystem->OnDestroyComplete.AddUniqueDynamic(this, &ThisClass::HandleSessionDestroyCompleted);
	}
}

void UPDRoomSessionFlowSubsystem::Deinitialize()
{
	if (UPDRoomSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UPDRoomSaveSubsystem>())
	{
		SaveSubsystem->OnRoomSaveCompleted.RemoveDynamic(this, &ThisClass::HandleRoomSaveCompleted);
	}

	if (UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem())
	{
		SessionSubsystem->OnCreateComplete.RemoveDynamic(this, &ThisClass::HandleSessionCreateCompleted);
		SessionSubsystem->OnFindComplete.RemoveDynamic(this, &ThisClass::HandleSessionFindCompleted);
		SessionSubsystem->OnJoinComplete.RemoveDynamic(this, &ThisClass::HandleSessionJoinCompleted);
		SessionSubsystem->OnInviteAccepted.RemoveDynamic(this, &ThisClass::HandleSteamInviteAccepted);
		SessionSubsystem->OnDestroyComplete.RemoveDynamic(this, &ThisClass::HandleSessionDestroyCompleted);
	}

	bHostExitRequest = false;
	FlowState = EPDRoomSessionFlowState::Idle;
	Super::Deinitialize();
}

bool UPDRoomSessionFlowSubsystem::RequestCreateRoom(const FString& RoomName)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Room creation request was ignored because another room flow operation is in progress."));
		return false;
	}

	UPDRoomSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UPDRoomSaveSubsystem>();
	if (!SaveSubsystem || SaveSubsystem->IsOperationInProgress())
	{
		const FString Error = TEXT("A room save context cannot be prepared while a save operation is in progress.");
		UE_LOG(LogPDServer, Warning, TEXT("Cannot create a room: %s"), *Error);
		CompleteCreate(false, Error);
		return false;
	}

	const FString TrimmedRoomName = RoomName.TrimStartAndEnd().IsEmpty() ? TEXT("PADO Room") : RoomName.TrimStartAndEnd();
	SetFlowState(EPDRoomSessionFlowState::PreparingNewRoom);
	FString RoomSaveId;
	if (!SaveSubsystem->BeginNewRoom(TrimmedRoomName, RoomSaveId))
	{
		const FString Error = TEXT("The new room save context could not be created.");
		UE_LOG(LogPDServer, Error, TEXT("Cannot create a room: %s"), *Error);
		CompleteCreate(false, Error);
		return false;
	}

	return StartSteamSessionCreation(TrimmedRoomName);
}

bool UPDRoomSessionFlowSubsystem::RequestFindRooms(const int32 MaxResults)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Room search request was ignored because another room flow operation is in progress."));
		return false;
	}

	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem();
	const FString ProductId = GetDefault<UPDSteamSessionSettings>()->ProductId.TrimStartAndEnd();
	if (!SessionSubsystem || ProductId.IsEmpty())
	{
		const FString Error = TEXT("The Steam session subsystem or PADO ProductId is unavailable.");
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot search for rooms: %s"), *Error);
		CompleteFind(false, {});
		return false;
	}

	SetFlowState(EPDRoomSessionFlowState::SearchingForSessions);
	SessionSubsystem->FindSessions(ProductId, FMath::Clamp(MaxResults, 1, 100), false);
	return true;
}

bool UPDRoomSessionFlowSubsystem::RequestJoinRoom(const FBlueprintSessionResult& Session)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Room join request was ignored because another room flow operation is in progress."));
		return false;
	}

	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem();
	if (!SessionSubsystem)
	{
		const FString Error = TEXT("The Steam session subsystem is unavailable.");
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot join a room: %s"), *Error);
		CompleteJoin(false, Error);
		return false;
	}

	SetFlowState(EPDRoomSessionFlowState::JoiningSteamSession);
	SessionSubsystem->JoinSession(Session);
	return true;
}

bool UPDRoomSessionFlowSubsystem::RequestReturnToMainMenu()
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Room exit request was ignored because another room flow operation is in progress. State=%s"), *StaticEnum<EPDRoomSessionFlowState>()->GetNameStringByValue(static_cast<int64>(FlowState)));
		return false;
	}

	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem();
	if (!SessionSubsystem || !SessionSubsystem->HasActiveSession())
	{
		const FString Error = TEXT("No active Steam session is available to leave.");
		UE_LOG(LogPDSteamSession, Warning, TEXT("Cannot return to the main menu: %s"), *Error);
		OnRoomExitCompleted.Broadcast(false, false, Error);
		return false;
	}

	bHostExitRequest = GetHostingGameMode() != nullptr;
	if (!bHostExitRequest)
	{
		BeginSessionDestruction();
		return true;
	}

	UPDRoomSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UPDRoomSaveSubsystem>();
	APDGameMode* GameMode = GetHostingGameMode();
	if (!SaveSubsystem || !GameMode || SaveSubsystem->IsOperationInProgress() || SaveSubsystem->GetActiveRoomSaveId().IsEmpty())
	{
		const FString Error = TEXT("The host room cannot close because no idle active room save context is available.");
		UE_LOG(LogPDServer, Warning, TEXT("Cannot start host room exit: %s"), *Error);
		CompleteExit(false, Error);
		return false;
	}

	SetFlowState(EPDRoomSessionFlowState::SavingBeforeHostExit);
	if (!GameMode->RequestRoomSave())
	{
		const FString Error = TEXT("The host room save request could not be started.");
		UE_LOG(LogPDServer, Error, TEXT("Cannot start host room exit: %s"), *Error);
		CompleteExit(false, Error);
		return false;
	}

	UE_LOG(LogPDServer, Log, TEXT("Host room exit is waiting for the active room save to complete."));
	return true;
}

UReusableSteamSessionSubsystem* UPDRoomSessionFlowSubsystem::GetSteamSessionSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UReusableSteamSessionSubsystem>() : nullptr;
}

APDGameMode* UPDRoomSessionFlowSubsystem::GetHostingGameMode() const
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	return World && World->GetNetMode() != NM_Client ? Cast<APDGameMode>(World->GetAuthGameMode()) : nullptr;
}

FString UPDRoomSessionFlowSubsystem::GetListenServerMapPackageName() const
{
	const UPDSteamSessionSettings* Settings = GetDefault<UPDSteamSessionSettings>();
	return Settings ? Settings->ListenServerMap.GetLongPackageName() : FString();
}

FString UPDRoomSessionFlowSubsystem::GetMainMenuMapPackageName() const
{
	const FString MainMenuMap = UGameMapsSettings::GetGameDefaultMap();
	if (MainMenuMap.IsEmpty())
	{
		return FString();
	}

	const FSoftObjectPath MainMenuMapPath(MainMenuMap);
	const FString LongPackageName = MainMenuMapPath.GetLongPackageName();
	return LongPackageName.IsEmpty() ? MainMenuMap : LongPackageName;
}

bool UPDRoomSessionFlowSubsystem::StartSteamSessionCreation(const FString& RoomName)
{
	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem();
	const UPDSteamSessionSettings* Settings = GetDefault<UPDSteamSessionSettings>();
	PendingListenServerMap = GetListenServerMapPackageName();
	if (!SessionSubsystem || !Settings || Settings->ProductId.TrimStartAndEnd().IsEmpty() || PendingListenServerMap.IsEmpty())
	{
		const FString Error = TEXT("The Steam session subsystem, ProductId, or listen server map is unavailable.");
		UE_LOG(LogPDSteamSession, Error, TEXT("Cannot create a room: %s"), *Error);
		CompleteCreate(false, Error);
		return false;
	}

	FReusableSessionSettings SessionSettings;
	SessionSettings.ProductId = Settings->ProductId.TrimStartAndEnd();
	SessionSettings.RoomName = RoomName;
	SessionSettings.MaxPublicConnections = Settings->GetMaxPlayers();
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bUseLobbiesIfAvailable = true;

	SetFlowState(EPDRoomSessionFlowState::CreatingSteamSession);
	SessionSubsystem->CreateListenSession(SessionSettings);
	return true;
}

void UPDRoomSessionFlowSubsystem::CompleteCreate(const bool bSuccess, const FString& Error)
{
	PendingListenServerMap.Reset();
	SetFlowState(EPDRoomSessionFlowState::Idle);
	OnRoomCreateCompleted.Broadcast(bSuccess, Error);
}

void UPDRoomSessionFlowSubsystem::CompleteFind(const bool bSuccess, const TArray<FReusableSessionEntry>& Sessions)
{
	SetFlowState(EPDRoomSessionFlowState::Idle);
	OnRoomFindCompleted.Broadcast(bSuccess, Sessions);
}

void UPDRoomSessionFlowSubsystem::CompleteJoin(const bool bSuccess, const FString& ConnectStringOrError)
{
	SetFlowState(EPDRoomSessionFlowState::Idle);
	OnRoomJoinCompleted.Broadcast(bSuccess, ConnectStringOrError);
}

void UPDRoomSessionFlowSubsystem::BeginSessionDestruction()
{
	UReusableSteamSessionSubsystem* SessionSubsystem = GetSteamSessionSubsystem();
	if (!SessionSubsystem)
	{
		CompleteExit(false, TEXT("The Steam session subsystem is unavailable."));
		return;
	}

	SetFlowState(EPDRoomSessionFlowState::DestroyingSession);
	SessionSubsystem->DestroySession();
}

void UPDRoomSessionFlowSubsystem::ReturnToMainMenu()
{
	const FString MainMenuMap = GetMainMenuMapPackageName();
	if (MainMenuMap.IsEmpty())
	{
		CompleteExit(false, TEXT("The default main menu map is not configured."));
		return;
	}

	SetFlowState(EPDRoomSessionFlowState::ReturningToMainMenu);
	if (bHostExitRequest)
	{
		UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		if (!World || !World->ServerTravel(MainMenuMap))
		{
			CompleteExit(false, TEXT("The host could not start travel to the main menu map."));
			return;
		}

		if (UPDRoomSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UPDRoomSaveSubsystem>())
		{
			SaveSubsystem->ClearActiveRoomContext();
		}
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(*MainMenuMap));
	}

	CompleteExit(true, FString());
}

void UPDRoomSessionFlowSubsystem::CompleteExit(const bool bSuccess, const FString& Error)
{
	const bool bWasHostExit = bHostExitRequest;
	bHostExitRequest = false;
	SetFlowState(EPDRoomSessionFlowState::Idle);

	if (bSuccess)
	{
		UE_LOG(LogPDSteamSession, Log, TEXT("Room exit flow completed. HostExit=%s"), bWasHostExit ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Room exit flow failed. HostExit=%s Error=%s"), bWasHostExit ? TEXT("true") : TEXT("false"), *Error);
	}

	OnRoomExitCompleted.Broadcast(bSuccess, bWasHostExit, Error);
}

void UPDRoomSessionFlowSubsystem::SetFlowState(const EPDRoomSessionFlowState NewState)
{
	if (FlowState == NewState)
	{
		return;
	}

	FlowState = NewState;
	OnFlowStateChanged.Broadcast(FlowState);
}

void UPDRoomSessionFlowSubsystem::HandleRoomSaveCompleted(const bool bSuccess, const FString&, const FString& Error)
{
	if (FlowState != EPDRoomSessionFlowState::SavingBeforeHostExit)
	{
		return;
	}

	if (!bSuccess)
	{
		CompleteExit(false, Error.IsEmpty() ? TEXT("The host room save failed before session destruction.") : Error);
		return;
	}

	UE_LOG(LogPDServer, Log, TEXT("Host room save completed. Starting Steam session destruction."));
	BeginSessionDestruction();
}

void UPDRoomSessionFlowSubsystem::HandleSessionCreateCompleted(const bool bSuccess, const FString& Error)
{
	if (FlowState != EPDRoomSessionFlowState::CreatingSteamSession)
	{
		return;
	}

	if (!bSuccess)
	{
		CompleteCreate(false, Error.IsEmpty() ? TEXT("The Steam session could not be created.") : Error);
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || PendingListenServerMap.IsEmpty() || !World->ServerTravel(PendingListenServerMap + TEXT("?listen")))
	{
		CompleteCreate(false, TEXT("The host could not start travel to the listen server map."));
		return;
	}

	SetFlowState(EPDRoomSessionFlowState::TravellingToGame);
	UE_LOG(LogPDSteamSession, Log, TEXT("Steam room creation succeeded. Starting listen server travel. Map=%s"), *PendingListenServerMap);
	CompleteCreate(true, FString());
}

void UPDRoomSessionFlowSubsystem::HandleSessionFindCompleted(const bool bSuccess, const TArray<FReusableSessionEntry>& Sessions)
{
	if (FlowState != EPDRoomSessionFlowState::SearchingForSessions)
	{
		return;
	}

	CompleteFind(bSuccess, Sessions);
}

void UPDRoomSessionFlowSubsystem::HandleSessionJoinCompleted(const bool bSuccess, const FString& ConnectStringOrError)
{
	if (FlowState != EPDRoomSessionFlowState::JoiningSteamSession)
	{
		return;
	}

	if (!bSuccess)
	{
		CompleteJoin(false, ConnectStringOrError.IsEmpty() ? TEXT("The Steam session could not be joined.") : ConnectStringOrError);
		return;
	}

	APlayerController* PlayerController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!PlayerController)
	{
		CompleteJoin(false, TEXT("No local player controller is available for client travel."));
		return;
	}

	SetFlowState(EPDRoomSessionFlowState::TravellingToGame);
	PlayerController->ClientTravel(ConnectStringOrError, ETravelType::TRAVEL_Absolute);
	UE_LOG(LogPDSteamSession, Log, TEXT("Steam room join succeeded. Starting client travel. ConnectString='%s'"), *ConnectStringOrError);
	CompleteJoin(true, ConnectStringOrError);
}

void UPDRoomSessionFlowSubsystem::HandleSteamInviteAccepted(const FBlueprintSessionResult&)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Steam invite join was ignored because another room flow operation is in progress."));
		return;
	}

	SetFlowState(EPDRoomSessionFlowState::JoiningSteamSession);
	OnRoomInviteAccepted.Broadcast();
	UE_LOG(LogPDSteamSession, Log, TEXT("Steam invite accepted. Waiting for the invite session join result."));
}

void UPDRoomSessionFlowSubsystem::HandleSessionDestroyCompleted(const bool bSuccess, const FString& Error)
{
	if (FlowState != EPDRoomSessionFlowState::DestroyingSession)
	{
		return;
	}

	if (!bSuccess)
	{
		CompleteExit(false, Error.IsEmpty() ? TEXT("The Steam session could not be destroyed.") : Error);
		return;
	}

	ReturnToMainMenu();
}
