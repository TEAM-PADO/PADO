// Copyright PADO. All Rights Reserved.

#include "PADO/SteamSession/PDRoomSessionFlowSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/OnlineReplStructs.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GameMapsSettings.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Base64.h"
#include "Misc/SecureHash.h"
#include "OnlineSubsystem.h"
#include "PADO/Core/PDGameMode.h"
#include "PADO/PADO.h"
#include "PADO/Save/PDRoomSaveSubsystem.h"
#include "PADO/SteamSession/PDSteamSessionSettings.h"
#include "ReusableSteamSessionSubsystem.h"

namespace PDRoomSessionFlow
{
	FString CalculatePasswordHash(const FString& Password)
	{
		const FTCHARToUTF8 PasswordUtf8(*Password);
		const FSHAHash PasswordHash = FSHA1::HashBuffer(PasswordUtf8.Get(), PasswordUtf8.Length());
		return PasswordHash.ToString();
	}
}

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

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddWeakLambda(this, [this](UWorld*, UNetDriver*, auto, const FString& Error)
		{
			HandleJoinNetworkFailure(Error);
		});
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

	if (GEngine && NetworkFailureHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		NetworkFailureHandle.Reset();
	}

	bHostExitRequest = false;
	bClientJoinTravelInProgress = false;
	PendingJoinEncodedPassword.Reset();
	PendingRoomName.Reset();
	ClearActiveRoomAccess();
	FlowState = EPDRoomSessionFlowState::Idle;
	Super::Deinitialize();
}

bool UPDRoomSessionFlowSubsystem::RequestCreateRoom(const FString& RoomName)
{
	return RequestCreateRoomInternal(RoomName, FPDRoomAccessSettings());
}

bool UPDRoomSessionFlowSubsystem::RequestCreateRoomWithAccessSettings(const FString& RoomName, const FPDRoomAccessSettings& AccessSettings)
{
	return RequestCreateRoomInternal(RoomName, AccessSettings);
}

bool UPDRoomSessionFlowSubsystem::RequestCreateRoomInternal(const FString& RoomName, const FPDRoomAccessSettings& AccessSettings)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Room creation request was ignored because another room flow operation is in progress."));
		return false;
	}

	FString AccessError;
	if (!ConfigureActiveRoomAccess(AccessSettings, AccessError))
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Cannot create a room because the access settings are invalid: %s"), *AccessError);
		CompleteCreate(false, AccessError);
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

	if (ActiveRoomAccessPolicy == EPDRoomAccessPolicy::FriendsOrPassword)
	{
		return BeginHostFriendsListRead(TrimmedRoomName);
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
	return RequestJoinRoomInternal(Session, FString());
}

bool UPDRoomSessionFlowSubsystem::RequestJoinRoomWithPassword(const FBlueprintSessionResult& Session, const FString& Password)
{
	return RequestJoinRoomInternal(Session, Password);
}

bool UPDRoomSessionFlowSubsystem::RequestJoinRoomInternal(const FBlueprintSessionResult& Session, const FString& Password)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Room join request was ignored because another room flow operation is in progress."));
		return false;
	}

	LastJoinConnectionError.Reset();
	bClientJoinTravelInProgress = false;
	if (!Password.IsEmpty())
	{
		FString PasswordError;
		if (!FPDRoomAccessSettings::ValidatePassword(Password, PasswordError))
		{
			UE_LOG(LogPDSteamSession, Warning, TEXT("Room join request was rejected because the submitted access code is invalid: %s"), *PasswordError);
			CompleteJoin(false, PasswordError);
			return false;
		}
	}

	PendingJoinEncodedPassword = Password.IsEmpty() ? FString() : FBase64::Encode(Password, EBase64Mode::UrlSafe);

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
		bClientJoinTravelInProgress = false;
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

bool UPDRoomSessionFlowSubsystem::ConfigureActiveRoomAccess(const FPDRoomAccessSettings& AccessSettings, FString& OutError)
{
	OutError.Reset();
	if (!AccessSettings.Validate(OutError))
	{
		return false;
	}

	ClearActiveRoomAccess();
	ActiveRoomAccessPolicy = AccessSettings.AccessPolicy;
	if (ActiveRoomAccessPolicy == EPDRoomAccessPolicy::FriendsOrPassword)
	{
		ActiveRoomPasswordHash = PDRoomSessionFlow::CalculatePasswordHash(AccessSettings.Password);
	}

	return true;
}

void UPDRoomSessionFlowSubsystem::ClearActiveRoomAccess()
{
	ActiveRoomAccessPolicy = EPDRoomAccessPolicy::Public;
	ActiveRoomPasswordHash.Reset();
	bHostFriendsListReady = false;
}

bool UPDRoomSessionFlowSubsystem::BeginHostFriendsListRead(const FString& RoomName)
{
	PendingRoomName = RoomName;
	bHostFriendsListReady = false;

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	if (!FriendsInterface.IsValid() || !LocalPlayer)
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Steam friends are unavailable. FriendsOrPassword rooms will temporarily require the password."));
		PendingRoomName.Reset();
		return StartSteamSessionCreation(RoomName);
	}

	SetFlowState(EPDRoomSessionFlowState::PreparingNewRoom);
	const bool bReadStarted = FriendsInterface->ReadFriendsList(
		LocalPlayer->GetControllerId(),
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::HandleHostFriendsListRead));
	if (bReadStarted)
	{
		return true;
	}

	UE_LOG(LogPDSteamSession, Warning, TEXT("Steam friends list read could not start. FriendsOrPassword rooms will temporarily require the password."));
	PendingRoomName.Reset();
	return StartSteamSessionCreation(RoomName);
}

bool UPDRoomSessionFlowSubsystem::IsEncodedRoomAccessPasswordValid(const FString& EncodedPassword) const
{
	if (ActiveRoomAccessPolicy != EPDRoomAccessPolicy::FriendsOrPassword || ActiveRoomPasswordHash.IsEmpty())
	{
		return false;
	}

	FString Password;
	FString PasswordError;
	return FBase64::Decode(EncodedPassword, Password, EBase64Mode::UrlSafe) &&
		FPDRoomAccessSettings::ValidatePassword(Password, PasswordError) &&
		PDRoomSessionFlow::CalculatePasswordHash(Password).Equals(ActiveRoomPasswordHash, ESearchCase::CaseSensitive);
}

bool UPDRoomSessionFlowSubsystem::IsIncomingPlayerSteamFriend(const FUniqueNetIdRepl& PlayerId) const
{
	if (!bHostFriendsListReady || !PlayerId.IsValid())
	{
		return false;
	}

	const FUniqueNetIdPtr IncomingPlayerId = PlayerId.GetUniqueNetId();
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	return IncomingPlayerId.IsValid() && FriendsInterface.IsValid() && LocalPlayer &&
		FriendsInterface->IsFriend(LocalPlayer->GetControllerId(), *IncomingPlayerId, EFriendsLists::ToString(EFriendsLists::Default));
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
	PendingRoomName.Reset();
	if (!bSuccess)
	{
		ClearActiveRoomAccess();
	}
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
	PendingJoinEncodedPassword.Reset();
	if (!bSuccess)
	{
		bClientJoinTravelInProgress = false;
	}
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

		ClearActiveRoomAccess();
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

	FString TravelConnectString = ConnectStringOrError;
	if (!PendingJoinEncodedPassword.IsEmpty())
	{
		TravelConnectString += FString::Printf(TEXT("?%s=%s"), PDRoomAccessOptions::PasswordKey, *PendingJoinEncodedPassword);
	}

	SetFlowState(EPDRoomSessionFlowState::TravellingToGame);
	bClientJoinTravelInProgress = true;
	PlayerController->ClientTravel(TravelConnectString, ETravelType::TRAVEL_Absolute);
	UE_LOG(LogPDSteamSession, Log, TEXT("Steam room join succeeded. Starting client travel."));
	CompleteJoin(true, ConnectStringOrError);
}

void UPDRoomSessionFlowSubsystem::HandleSteamInviteAccepted(const FBlueprintSessionResult&)
{
	if (IsFlowOperationInProgress())
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Steam invite join was ignored because another room flow operation is in progress."));
		return;
	}

	LastJoinConnectionError.Reset();
	PendingJoinEncodedPassword.Reset();
	bClientJoinTravelInProgress = false;
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

void UPDRoomSessionFlowSubsystem::HandleHostFriendsListRead(const int32, const bool bSuccess, const FString&, const FString& Error)
{
	if (FlowState != EPDRoomSessionFlowState::PreparingNewRoom || PendingRoomName.IsEmpty())
	{
		return;
	}

	bHostFriendsListReady = bSuccess;
	if (!bSuccess)
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Steam friends list read failed. FriendsOrPassword rooms will temporarily require the password. Error=%s"), *Error);
	}

	const FString RoomName = MoveTemp(PendingRoomName);
	StartSteamSessionCreation(RoomName);
}

void UPDRoomSessionFlowSubsystem::HandleJoinNetworkFailure(const FString& Error)
{
	if (!bClientJoinTravelInProgress)
	{
		return;
	}

	bClientJoinTravelInProgress = false;
	LastJoinConnectionError = Error.IsEmpty() ? TEXT("The connection to the room failed.") : Error;
	SetFlowState(EPDRoomSessionFlowState::Idle);
	OnRoomJoinConnectionFailed.Broadcast(LastJoinConnectionError);
	UE_LOG(LogPDSteamSession, Warning, TEXT("Room connection failed after Steam session join: %s"), *LastJoinConnectionError);

	const FString MainMenuMap = GetMainMenuMapPackageName();
	if (MainMenuMap.IsEmpty())
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("The client could not return to the main menu because the default main menu map is not configured."));
		return;
	}

	UGameplayStatics::OpenLevel(this, FName(*MainMenuMap));
}
