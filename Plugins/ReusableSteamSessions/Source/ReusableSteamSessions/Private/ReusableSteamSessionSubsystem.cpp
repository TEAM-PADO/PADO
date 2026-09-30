#include "ReusableSteamSessionSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Base64.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

namespace ReusableSessionKeys
{
	const FName ProductId(TEXT("RSS_ProductId"));
	const FName RoomName(TEXT("RSS_RoomName"));
	const FName FriendsOnly(TEXT("RSS_FriendsOnly"));
}

void UReusableSteamSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("ReusableSteamSessions: no OnlineSession interface. Check DefaultPlatformService and enabled OSS plugins."));
		return;
	}

	CreateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateComplete));
	JoinHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinComplete));
	DestroyHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroyComplete));
	FindHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindComplete));
	InviteHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleInviteAccepted));
}

void UReusableSteamSessionSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteHandle);
	}
	ActiveSearch.Reset();
	SessionInterface.Reset();
	Super::Deinitialize();
}

ULocalPlayer* UReusableSteamSessionSubsystem::GetLocalPlayer() const
{
	return GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
}

void UReusableSteamSessionSubsystem::CreateListenSession(const FReusableSessionSettings& Settings)
{
	if (Operation != EOperation::None)
	{
		FailCreate(TEXT("Another session operation is already in progress."));
		return;
	}
	if (!SessionInterface.IsValid())
	{
		FailCreate(TEXT("Online session interface is unavailable."));
		return;
	}
	if (!GetLocalPlayer())
	{
		FailCreate(TEXT("No local player is available."));
		return;
	}

	PendingCreateSettings = Settings;
	PendingCreateSettings.ProductId = PendingCreateSettings.ProductId.TrimStartAndEnd();
	PendingCreateSettings.RoomName = PendingCreateSettings.RoomName.TrimStartAndEnd();
	PendingCreateSettings.MaxPublicConnections = FMath::Clamp(PendingCreateSettings.MaxPublicConnections, 1, 100);
	if (PendingCreateSettings.ProductId.IsEmpty())
	{
		FailCreate(TEXT("ProductId must not be empty."));
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		Operation = EOperation::Destroying;
		bCreateAfterDestroy = true;
		if (!SessionInterface->DestroySession(NAME_GameSession))
		{
			bCreateAfterDestroy = false;
			Operation = EOperation::None;
			FailCreate(TEXT("Could not destroy the existing session before creation."));
		}
		return;
	}
	BeginCreate(PendingCreateSettings);
}

void UReusableSteamSessionSubsystem::BeginCreate(const FReusableSessionSettings& Settings)
{
	FOnlineSessionSettings OnlineSettings;
	OnlineSettings.bIsLANMatch = Settings.bIsLANMatch;
	OnlineSettings.NumPublicConnections = Settings.MaxPublicConnections;
	OnlineSettings.bAllowJoinInProgress = Settings.bAllowJoinInProgress;
	OnlineSettings.bAllowInvites = true;
	OnlineSettings.bShouldAdvertise = true;
	OnlineSettings.bUsesPresence = true;
	OnlineSettings.bAllowJoinViaPresence = true;
	OnlineSettings.bAllowJoinViaPresenceFriendsOnly = Settings.bFriendsOnly;
	OnlineSettings.bUseLobbiesIfAvailable = Settings.bUseLobbiesIfAvailable;
	OnlineSettings.Set(ReusableSessionKeys::ProductId, Settings.ProductId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	OnlineSettings.Set(ReusableSessionKeys::FriendsOnly, Settings.bFriendsOnly, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	// Steam OSS can corrupt non-ASCII advertised strings; encode room names before advertising.
	OnlineSettings.Set(ReusableSessionKeys::RoomName, FString::Printf(TEXT("B64:%s"), *FBase64::Encode(Settings.RoomName)), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	Operation = EOperation::Creating;
	if (!SessionInterface->CreateSession(GetLocalPlayer()->GetControllerId(), NAME_GameSession, OnlineSettings))
	{
		Operation = EOperation::None;
		FailCreate(TEXT("CreateSession was rejected immediately by the online subsystem."));
	}
}

void UReusableSteamSessionSubsystem::FindSessions(const FString& ProductId, const int32 MaxResults, const bool bIncludeFriendsOnly)
{
	if (Operation != EOperation::None || !SessionInterface.IsValid() || !GetLocalPlayer())
	{
		OnFindComplete.Broadcast(false, {});
		return;
	}
	const FString TrimmedProductId = ProductId.TrimStartAndEnd();
	if (TrimmedProductId.IsEmpty())
	{
		OnFindComplete.Broadcast(false, {});
		return;
	}

	ActiveSearch = MakeShared<FOnlineSessionSearch>();
	ActiveSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 100);
	ActiveSearch->bIsLanQuery = false;
	ActiveSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	// ProductId is filtered again in HandleFindComplete because not every OSS supports arbitrary query keys.
	PendingCreateSettings.ProductId = TrimmedProductId;
	PendingCreateSettings.bFriendsOnly = bIncludeFriendsOnly;
	Operation = EOperation::Finding;
	if (!SessionInterface->FindSessions(GetLocalPlayer()->GetControllerId(), ActiveSearch.ToSharedRef()))
	{
		Operation = EOperation::None;
		OnFindComplete.Broadcast(false, {});
	}
}

void UReusableSteamSessionSubsystem::JoinSession(const FBlueprintSessionResult& Session)
{
	if (Operation != EOperation::None)
	{
		FailJoin(TEXT("Another session operation is already in progress."));
		return;
	}
	if (!SessionInterface.IsValid() || !GetLocalPlayer())
	{
		FailJoin(TEXT("Online session interface or local player is unavailable."));
		return;
	}
	if (!Session.OnlineResult.IsValid())
	{
		FailJoin(TEXT("The selected session result is invalid or expired."));
		return;
	}
	if (Session.OnlineResult.Session.NumOpenPublicConnections <= 0)
	{
		FailJoin(TEXT("The session is full."));
		return;
	}

	PendingJoinResult = Session;
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		Operation = EOperation::Destroying;
		bJoinAfterDestroy = true;
		if (!SessionInterface->DestroySession(NAME_GameSession))
		{
			bJoinAfterDestroy = false;
			Operation = EOperation::None;
			FailJoin(TEXT("Could not leave the existing session before joining."));
		}
		return;
	}

	Operation = EOperation::Joining;
	if (!SessionInterface->JoinSession(GetLocalPlayer()->GetControllerId(), NAME_GameSession, Session.OnlineResult))
	{
		Operation = EOperation::None;
		FailJoin(TEXT("JoinSession was rejected immediately by the online subsystem."));
	}
}

void UReusableSteamSessionSubsystem::DestroySession()
{
	if (Operation != EOperation::None)
	{
		OnDestroyComplete.Broadcast(false, TEXT("Another session operation is already in progress."));
		return;
	}
	if (!SessionInterface.IsValid())
	{
		OnDestroyComplete.Broadcast(false, TEXT("Online session interface is unavailable."));
		return;
	}
	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		OnDestroyComplete.Broadcast(true, FString());
		return;
	}
	Operation = EOperation::Destroying;
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		Operation = EOperation::None;
		OnDestroyComplete.Broadcast(false, TEXT("DestroySession was rejected immediately by the online subsystem."));
	}
}

bool UReusableSteamSessionSubsystem::InviteFriend(const FString& FriendUniqueNetId)
{
	if (!SessionInterface.IsValid() || IsCurrentSessionFull() || !GetLocalPlayer())
	{
		return false;
	}
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr Identity = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
	FUniqueNetIdPtr FriendId = Identity.IsValid() ? Identity->CreateUniquePlayerId(FriendUniqueNetId) : nullptr;
	return FriendId.IsValid() && SessionInterface->SendSessionInviteToFriend(GetLocalPlayer()->GetControllerId(), NAME_GameSession, *FriendId);
}

bool UReusableSteamSessionSubsystem::IsCurrentSessionFull() const
{
	const FNamedOnlineSession* Session = SessionInterface.IsValid() ? SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	return Session && Session->NumOpenPublicConnections <= 0;
}

void UReusableSteamSessionSubsystem::HandleCreateComplete(FName, const bool bSuccess)
{
	Operation = EOperation::None;
	if (bSuccess && !PendingCreateSettings.ListenServerMap.TrimStartAndEnd().IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(PendingCreateSettings.ListenServerMap.TrimStartAndEnd() + TEXT("?listen"));
		}
	}
	OnCreateComplete.Broadcast(bSuccess, bSuccess ? FString() : TEXT("The online subsystem failed to create the session."));
}

void UReusableSteamSessionSubsystem::HandleJoinComplete(FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	Operation = EOperation::None;
	FString ConnectString;
	if (Result == EOnJoinSessionCompleteResult::Success && SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
		{
			PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
		}
		OnJoinComplete.Broadcast(true, ConnectString);
		return;
	}
	const FString Error = Result == EOnJoinSessionCompleteResult::SessionIsFull ? TEXT("The session is full.") : TEXT("Unable to resolve or join the session.");
	FailJoin(Error);
}

void UReusableSteamSessionSubsystem::HandleDestroyComplete(FName, const bool bSuccess)
{
	Operation = EOperation::None;
	if (bCreateAfterDestroy)
	{
		bCreateAfterDestroy = false;
		if (bSuccess) { BeginCreate(PendingCreateSettings); } else { FailCreate(TEXT("Could not destroy the existing session before creation.")); }
		return;
	}
	if (bJoinAfterDestroy)
	{
		bJoinAfterDestroy = false;
		if (bSuccess) { JoinSession(PendingJoinResult); } else { FailJoin(TEXT("Could not leave the existing session before joining.")); }
		return;
	}
	OnDestroyComplete.Broadcast(bSuccess, bSuccess ? FString() : TEXT("The online subsystem failed to destroy the session."));
}

void UReusableSteamSessionSubsystem::HandleFindComplete(const bool bSuccess)
{
	Operation = EOperation::None;
	TArray<FReusableSessionEntry> Entries;
	if (bSuccess && ActiveSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : ActiveSearch->SearchResults)
		{
			FString ProductId;
			bool bFriendsOnly = false;
			if (!Result.IsValid() || !Result.Session.SessionSettings.Get(ReusableSessionKeys::ProductId, ProductId) || ProductId != PendingCreateSettings.ProductId ||
				(Result.Session.SessionSettings.Get(ReusableSessionKeys::FriendsOnly, bFriendsOnly) && bFriendsOnly && !PendingCreateSettings.bFriendsOnly)) continue;
			FReusableSessionEntry& Entry = Entries.AddDefaulted_GetRef();
			FString EncodedName;
			Result.Session.SessionSettings.Get(ReusableSessionKeys::RoomName, EncodedName);
			Entry.RoomName = EncodedName.StartsWith(TEXT("B64:")) && FBase64::Decode(EncodedName.RightChop(4), Entry.RoomName) ? Entry.RoomName : EncodedName;
			if (Entry.RoomName.IsEmpty()) Entry.RoomName = Result.Session.OwningUserName;
			Entry.HostName = Result.Session.OwningUserName;
			Entry.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Entry.CurrentPlayers = FMath::Clamp(Entry.MaxPlayers - Result.Session.NumOpenPublicConnections, 0, Entry.MaxPlayers);
			Entry.PingInMs = Result.PingInMs;
			Entry.bFriendsOnly = bFriendsOnly;
			Entry.SessionResult.OnlineResult = Result;
		}
	}
	OnFindComplete.Broadcast(bSuccess, Entries);
}

void UReusableSteamSessionSubsystem::HandleInviteAccepted(const bool bSuccess, int32, FUniqueNetIdPtr, const FOnlineSessionSearchResult& Result)
{
	if (!bSuccess || !Result.IsValid()) return;
	FBlueprintSessionResult BlueprintResult;
	BlueprintResult.OnlineResult = Result;
	OnInviteAccepted.Broadcast(BlueprintResult);
	JoinSession(BlueprintResult);
}

void UReusableSteamSessionSubsystem::FailCreate(const FString& Error) { OnCreateComplete.Broadcast(false, Error); }
void UReusableSteamSessionSubsystem::FailJoin(const FString& Error) { OnJoinComplete.Broadcast(false, Error); }
