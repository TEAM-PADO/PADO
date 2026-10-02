#include "PADO/SteamSession/PDSteamSessionDebugSubsystem.h"

#include "HAL/IConsoleManager.h"
#include "PADO/PADO.h"
#include "PADO/SteamSession/PDRoomSessionFlowSubsystem.h"
#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"
#include "ReusableSteamSessionSubsystem.h"

void UPDSteamSessionDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	Collection.InitializeDependency<UReusableSteamSessionSubsystem>();

	if (UReusableSteamSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->OnCreateComplete.AddDynamic(this, &ThisClass::HandleCreateComplete);
		SessionSubsystem->OnFindComplete.AddDynamic(this, &ThisClass::HandleFindComplete);
		SessionSubsystem->OnJoinComplete.AddDynamic(this, &ThisClass::HandleJoinComplete);
		SessionSubsystem->OnDestroyComplete.AddDynamic(this, &ThisClass::HandleDestroyComplete);
	}
	else
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Steam session debug commands are unavailable because the session subsystem could not be initialized."));
	}

	RegisterConsoleCommands();
#endif
}

void UPDSteamSessionDebugSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	UnregisterConsoleCommands();

	if (UReusableSteamSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->OnCreateComplete.RemoveDynamic(this, &ThisClass::HandleCreateComplete);
		SessionSubsystem->OnFindComplete.RemoveDynamic(this, &ThisClass::HandleFindComplete);
		SessionSubsystem->OnJoinComplete.RemoveDynamic(this, &ThisClass::HandleJoinComplete);
		SessionSubsystem->OnDestroyComplete.RemoveDynamic(this, &ThisClass::HandleDestroyComplete);
	}

	LastFoundSessions.Reset();
#endif

	Super::Deinitialize();
}

#if !UE_BUILD_SHIPPING
void UPDSteamSessionDebugSubsystem::RegisterConsoleCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	if (ConsoleManager.FindConsoleObject(TEXT("PD.Steam.Create")))
	{
		UE_LOG(LogPDSteamSession, Warning, TEXT("Steam session debug commands are already registered by another GameInstance."));
		return;
	}

	CreateCommand = ConsoleManager.RegisterConsoleCommand(
		TEXT("PD.Steam.Create"),
		TEXT("Creates a PADO Steam listen session. Optional argument: room name."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::ExecuteCreateCommand),
		ECVF_Default);
	FindCommand = ConsoleManager.RegisterConsoleCommand(
		TEXT("PD.Steam.Find"),
		TEXT("Finds public PADO Steam sessions and writes indexed results to LogPDSteamSession."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::ExecuteFindCommand),
		ECVF_Default);
	JoinCommand = ConsoleManager.RegisterConsoleCommand(
		TEXT("PD.Steam.Join"),
		TEXT("Joins a session index returned by PD.Steam.Find. Usage: PD.Steam.Join <Index>"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::ExecuteJoinCommand),
		ECVF_Default);
	DestroyCommand = ConsoleManager.RegisterConsoleCommand(
		TEXT("PD.Steam.Destroy"),
		TEXT("Destroys the current PADO Steam session."),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::ExecuteDestroyCommand),
		ECVF_Default);
}

void UPDSteamSessionDebugSubsystem::UnregisterConsoleCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();
	if (CreateCommand)
	{
		ConsoleManager.UnregisterConsoleObject(CreateCommand);
		CreateCommand = nullptr;
	}
	if (FindCommand)
	{
		ConsoleManager.UnregisterConsoleObject(FindCommand);
		FindCommand = nullptr;
	}
	if (JoinCommand)
	{
		ConsoleManager.UnregisterConsoleObject(JoinCommand);
		JoinCommand = nullptr;
	}
	if (DestroyCommand)
	{
		ConsoleManager.UnregisterConsoleObject(DestroyCommand);
		DestroyCommand = nullptr;
	}
}

UReusableSteamSessionSubsystem* UPDSteamSessionDebugSubsystem::GetSessionSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UReusableSteamSessionSubsystem>() : nullptr;
}

void UPDSteamSessionDebugSubsystem::ExecuteCreateCommand(const TArray<FString>& Arguments)
{
	const FString RoomName = Arguments.IsEmpty() ? TEXT("PADO Debug Room") : FString::Join(Arguments, TEXT(" "));
	if (!UPDSteamSessionBlueprintLibrary::CreatePADOListenSession(this, RoomName))
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("PD.Steam.Create could not start a Steam session request. Check prior log messages."));
	}
}

void UPDSteamSessionDebugSubsystem::ExecuteFindCommand(const TArray<FString>&)
{
	LastFoundSessions.Reset();
	if (!UPDSteamSessionBlueprintLibrary::FindPADOSessions(this))
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("PD.Steam.Find could not start a Steam session search. Check prior log messages."));
	}
}

void UPDSteamSessionDebugSubsystem::ExecuteJoinCommand(const TArray<FString>& Arguments)
{
	if (Arguments.Num() != 1)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Usage: PD.Steam.Join <Index>. Run PD.Steam.Find first."));
		return;
	}

	int32 SessionIndex = INDEX_NONE;
	if (!LexTryParseString(SessionIndex, *Arguments[0]) || !LastFoundSessions.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Session index '%s' is invalid. Run PD.Steam.Find and select a displayed index."), *Arguments[0]);
		return;
	}

	if (!UPDSteamSessionBlueprintLibrary::JoinPADOSession(this, LastFoundSessions[SessionIndex]))
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("PD.Steam.Join could not start a Steam session join request. Check prior log messages."));
	}
}

void UPDSteamSessionDebugSubsystem::ExecuteDestroyCommand(const TArray<FString>&)
{
	UPDRoomSessionFlowSubsystem* FlowSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPDRoomSessionFlowSubsystem>() : nullptr;
	if (!FlowSubsystem)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("PD.Steam.Destroy could not find the room session flow subsystem."));
		return;
	}

	if (!FlowSubsystem->RequestReturnToMainMenu())
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("PD.Steam.Destroy could not start the room exit flow. Check prior log messages."));
	}
}

#endif

void UPDSteamSessionDebugSubsystem::HandleCreateComplete(const bool bSuccess, const FString& Error)
{
	if (bSuccess)
	{
		UE_LOG(LogPDSteamSession, Display, TEXT("Steam session create succeeded."));
		return;
	}

	UE_LOG(LogPDSteamSession, Error, TEXT("Steam session create failed: %s"), *Error);
}

void UPDSteamSessionDebugSubsystem::HandleFindComplete(const bool bSuccess, const TArray<FReusableSessionEntry>& Sessions)
{
	LastFoundSessions.Reset();
	if (!bSuccess)
	{
		UE_LOG(LogPDSteamSession, Error, TEXT("Steam session search failed."));
		return;
	}

	UE_LOG(LogPDSteamSession, Display, TEXT("Steam session search completed with %d public PADO room(s)."), Sessions.Num());
	for (int32 Index = 0; Index < Sessions.Num(); ++Index)
	{
		const FReusableSessionEntry& Entry = Sessions[Index];
		LastFoundSessions.Add(Entry.SessionResult);
		UE_LOG(LogPDSteamSession, Display, TEXT("[%d] Room='%s' Host='%s' Players=%d/%d Ping=%dms"), Index, *Entry.RoomName, *Entry.HostName, Entry.CurrentPlayers, Entry.MaxPlayers, Entry.PingInMs);
	}
}

void UPDSteamSessionDebugSubsystem::HandleJoinComplete(const bool bSuccess, const FString& ConnectString)
{
	if (bSuccess)
	{
		UE_LOG(LogPDSteamSession, Display, TEXT("Steam session join succeeded. ConnectString='%s'"), *ConnectString);
		return;
	}

	UE_LOG(LogPDSteamSession, Error, TEXT("Steam session join failed."));
}

void UPDSteamSessionDebugSubsystem::HandleDestroyComplete(const bool bSuccess, const FString& Error)
{
	if (bSuccess)
	{
		UE_LOG(LogPDSteamSession, Display, TEXT("Steam session destroy succeeded."));
		return;
	}

	UE_LOG(LogPDSteamSession, Error, TEXT("Steam session destroy failed: %s"), *Error);
}
