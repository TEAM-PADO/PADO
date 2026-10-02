// Copyright PADO. All Rights Reserved.

#include "PADO/Save/PDRoomSaveSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "PADO/PADO.h"
#include "PADO/Save/PDRoomSaveGame.h"
#include "PADO/Save/PDRoomSaveIndexSaveGame.h"

bool UPDRoomSaveSubsystem::BeginNewRoom(const FString& RoomDisplayName, FString& OutRoomSaveId)
{
	OutRoomSaveId.Reset();

	if (IsOperationInProgress())
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot begin a new room while a save operation is in progress."));
		return false;
	}

	FString PlatformUserId;
	if (!ResolveLocalPlatformUserId(PlatformUserId))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot begin a new room because the local platform user ID is unavailable."));
		return false;
	}

	ActiveRoomSaveId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	ActiveHostPlatformUserId = MoveTemp(PlatformUserId);
	ActiveRoomDisplayName = RoomDisplayName.TrimStartAndEnd();
	if (ActiveRoomDisplayName.IsEmpty())
	{
		ActiveRoomDisplayName = TEXT("Untitled Room");
	}

	ActiveRoomSaveGame = nullptr;
	QueuedPersistentState.Reset();
	OutRoomSaveId = ActiveRoomSaveId;
	UE_LOG(LogPDServer, Log, TEXT("Created a new room save context. RoomSaveId=%s"), *ActiveRoomSaveId);
	return true;
}

bool UPDRoomSaveSubsystem::LoadRoomForResume(const FString& RoomSaveId)
{
	FString ValidationError;
	if (!ValidateRoomSaveId(RoomSaveId, ValidationError))
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot load room save: %s"), *ValidationError);
		return false;
	}

	if (IsOperationInProgress())
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot load a room save while another save operation is in progress."));
		return false;
	}

	FString PlatformUserId;
	if (!ResolveLocalPlatformUserId(PlatformUserId))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot load a room save because the local platform user ID is unavailable."));
		return false;
	}

	PendingLoadRoomSaveId = RoomSaveId;
	ActiveHostPlatformUserId = MoveTemp(PlatformUserId);
	Operation = EOperation::LoadingRoom;
	UGameplayStatics::AsyncLoadGameFromSlot(
		GetRoomSaveSlotName(RoomSaveId),
		SaveUserIndex,
		FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &ThisClass::HandleRoomLoadCompleted));
	return true;
}

bool UPDRoomSaveSubsystem::SaveActiveRoom(const FPDRoomPersistentState& PersistentState)
{
	FString ValidationError;
	if (!PersistentState.Validate(ValidationError))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot save the active room because the persistent state is invalid: %s"), *ValidationError);
		return false;
	}

	if (ActiveRoomSaveId.IsEmpty() || ActiveHostPlatformUserId.IsEmpty())
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot save the active room because no room save context has been selected."));
		return false;
	}

	if (Operation == EOperation::SavingRoom || Operation == EOperation::SavingIndex)
	{
		QueuedPersistentState = PersistentState;
		UE_LOG(LogPDServer, Verbose, TEXT("Queued the latest room save request while another room save is in progress. RoomSaveId=%s"), *ActiveRoomSaveId);
		return true;
	}

	if (Operation != EOperation::None)
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot save the active room while a load operation is in progress."));
		return false;
	}

	return StartSaveActiveRoom(PersistentState);
}

bool UPDRoomSaveSubsystem::RefreshRoomSaveListEntries()
{
	if (IsOperationInProgress())
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot refresh room save list entries while a save operation is in progress."));
		return false;
	}

	FString PlatformUserId;
	if (!ResolveLocalPlatformUserId(PlatformUserId))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot refresh room save list entries because the local platform user ID is unavailable."));
		return false;
	}

	FString LoadError;
	UPDRoomSaveIndexSaveGame* IndexSaveGame = LoadIndexSaveGame(LoadError);
	if (!IndexSaveGame)
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot refresh room save list entries: %s"), *LoadError);
		return false;
	}

	UpdateCachedListEntries(*IndexSaveGame, PlatformUserId);
	return true;
}

TArray<FPDRoomSaveListEntry> UPDRoomSaveSubsystem::GetRoomSaveListEntries() const
{
	return CachedRoomSaveListEntries;
}

bool UPDRoomSaveSubsystem::DeleteRoomSave(const FString& RoomSaveId)
{
	FString ValidationError;
	if (!ValidateRoomSaveId(RoomSaveId, ValidationError))
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot delete room save: %s"), *ValidationError);
		return false;
	}

	if (IsOperationInProgress())
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot delete a room save while another save operation is in progress."));
		return false;
	}

	FString PlatformUserId;
	if (!ResolveLocalPlatformUserId(PlatformUserId))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot delete a room save because the local platform user ID is unavailable."));
		return false;
	}

	FString LoadError;
	UPDRoomSaveIndexSaveGame* IndexSaveGame = LoadIndexSaveGame(LoadError);
	if (!IndexSaveGame)
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot delete room save: %s"), *LoadError);
		return false;
	}

	const int32 ListEntryIndex = IndexSaveGame->FindListEntryIndex(RoomSaveId);
	if (ListEntryIndex == INDEX_NONE || IndexSaveGame->RoomSaveListEntries[ListEntryIndex].HostPlatformUserId != PlatformUserId)
	{
		UE_LOG(LogPDServer, Warning, TEXT("Cannot delete room save because it is missing or belongs to a different host. RoomSaveId=%s"), *RoomSaveId);
		return false;
	}

	if (!UGameplayStatics::DeleteGameInSlot(GetRoomSaveSlotName(RoomSaveId), SaveUserIndex))
	{
		const FString Error = TEXT("The room save file could not be deleted.");
		UE_LOG(LogPDServer, Error, TEXT("Cannot delete room save. RoomSaveId=%s Error=%s"), *RoomSaveId, *Error);
		OnRoomDeleteCompleted.Broadcast(false, RoomSaveId, Error);
		return false;
	}

	IndexSaveGame->RemoveListEntry(RoomSaveId);
	if (!UGameplayStatics::SaveGameToSlot(IndexSaveGame, GetIndexSaveSlotName(), SaveUserIndex))
	{
		const FString Error = TEXT("The room save index could not be updated after deleting the room save.");
		UE_LOG(LogPDServer, Error, TEXT("Room save file was deleted but the index update failed. RoomSaveId=%s"), *RoomSaveId);
		OnRoomDeleteCompleted.Broadcast(false, RoomSaveId, Error);
		return false;
	}

	UpdateCachedListEntries(*IndexSaveGame, PlatformUserId);
	if (ActiveRoomSaveId == RoomSaveId)
	{
		ClearActiveRoomContext();
	}

	UE_LOG(LogPDServer, Log, TEXT("Deleted room save. RoomSaveId=%s"), *RoomSaveId);
	OnRoomDeleteCompleted.Broadcast(true, RoomSaveId, FString());
	return true;
}

FString UPDRoomSaveSubsystem::GetActiveRoomSaveId() const
{
	return ActiveRoomSaveId;
}

UPDRoomSaveGame* UPDRoomSaveSubsystem::GetActiveRoomSaveGame() const
{
	return ActiveRoomSaveGame;
}

bool UPDRoomSaveSubsystem::IsOperationInProgress() const
{
	return Operation != EOperation::None;
}

void UPDRoomSaveSubsystem::ClearActiveRoomContext()
{
	if (IsOperationInProgress())
	{
		UE_LOG(LogPDServer, Warning, TEXT("Ignored ClearActiveRoomContext while a save operation is in progress."));
		return;
	}

	ActiveRoomSaveId.Reset();
	ActiveHostPlatformUserId.Reset();
	ActiveRoomDisplayName.Reset();
	ActiveRoomSaveGame = nullptr;
	QueuedPersistentState.Reset();
}

bool UPDRoomSaveSubsystem::ResolveLocalPlatformUserId(FString& OutPlatformUserId) const
{
	OutPlatformUserId.Reset();

	const UGameInstance* GameInstance = GetGameInstance();
	const ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	if (!LocalPlayer)
	{
		return false;
	}

	const FUniqueNetIdRepl UniqueNetId = LocalPlayer->GetPreferredUniqueNetId();
	if (!UniqueNetId.IsValid())
	{
		return false;
	}

	OutPlatformUserId = UniqueNetId.ToString();
	return !OutPlatformUserId.IsEmpty();
}

bool UPDRoomSaveSubsystem::ValidateRoomSaveId(const FString& RoomSaveId, FString& OutError) const
{
	OutError.Reset();

	FGuid ParsedRoomSaveId;
	if (!FGuid::Parse(RoomSaveId, ParsedRoomSaveId) || !ParsedRoomSaveId.IsValid())
	{
		OutError = TEXT("RoomSaveId is missing or invalid.");
		return false;
	}

	return true;
}

FString UPDRoomSaveSubsystem::GetRoomSaveSlotName(const FString& RoomSaveId) const
{
	return FString::Printf(TEXT("PADO_Room_%s"), *RoomSaveId);
}

FString UPDRoomSaveSubsystem::GetIndexSaveSlotName() const
{
	return TEXT("PADO_RoomIndex");
}

UPDRoomSaveIndexSaveGame* UPDRoomSaveSubsystem::LoadIndexSaveGame(FString& OutError) const
{
	OutError.Reset();
	const FString IndexSlotName = GetIndexSaveSlotName();
	if (!UGameplayStatics::DoesSaveGameExist(IndexSlotName, SaveUserIndex))
	{
		return Cast<UPDRoomSaveIndexSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDRoomSaveIndexSaveGame::StaticClass()));
	}

	UPDRoomSaveIndexSaveGame* IndexSaveGame = Cast<UPDRoomSaveIndexSaveGame>(UGameplayStatics::LoadGameFromSlot(IndexSlotName, SaveUserIndex));
	if (!IndexSaveGame)
	{
		OutError = TEXT("The room save index is missing, corrupted, or has an unexpected class.");
		return nullptr;
	}

	if (!IndexSaveGame->Validate(OutError))
	{
		return nullptr;
	}

	return IndexSaveGame;
}

bool UPDRoomSaveSubsystem::StartSaveActiveRoom(const FPDRoomPersistentState& PersistentState)
{
	UPDRoomSaveGame* SaveGame = Cast<UPDRoomSaveGame>(UGameplayStatics::CreateSaveGameObject(UPDRoomSaveGame::StaticClass()));
	if (!SaveGame)
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot create a room save object. RoomSaveId=%s"), *ActiveRoomSaveId);
		return false;
	}

	SaveGame->RoomSaveId = ActiveRoomSaveId;
	SaveGame->HostPlatformUserId = ActiveHostPlatformUserId;
	SaveGame->RoomDisplayName = ActiveRoomDisplayName;
	SaveGame->PersistentState = PersistentState;

	FString ValidationError;
	if (!SaveGame->Validate(ValidationError))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot save the active room because the generated save snapshot is invalid: %s"), *ValidationError);
		return false;
	}

	PendingRoomSaveGame = SaveGame;
	Operation = EOperation::SavingRoom;
	UGameplayStatics::AsyncSaveGameToSlot(
		SaveGame,
		GetRoomSaveSlotName(ActiveRoomSaveId),
		SaveUserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &ThisClass::HandleRoomSaveCompleted));
	return true;
}

void UPDRoomSaveSubsystem::StartQueuedSaveIfNeeded()
{
	if (!QueuedPersistentState.IsSet() || Operation != EOperation::None)
	{
		return;
	}

	const FPDRoomPersistentState LatestPersistentState = QueuedPersistentState.GetValue();
	QueuedPersistentState.Reset();
	if (!StartSaveActiveRoom(LatestPersistentState))
	{
		const FString Error = TEXT("The queued room save request could not be started.");
		UE_LOG(LogPDServer, Error, TEXT("%s RoomSaveId=%s"), *Error, *ActiveRoomSaveId);
		OnRoomSaveCompleted.Broadcast(false, ActiveRoomSaveId, Error);
	}
}

void UPDRoomSaveSubsystem::UpdateCachedListEntries(const UPDRoomSaveIndexSaveGame& IndexSaveGame, const FString& PlatformUserId)
{
	CachedRoomSaveListEntries.Reset();
	for (const FPDRoomSaveListEntry& ListEntry : IndexSaveGame.RoomSaveListEntries)
	{
		if (ListEntry.HostPlatformUserId == PlatformUserId)
		{
			CachedRoomSaveListEntries.Add(ListEntry);
		}
	}
}

void UPDRoomSaveSubsystem::HandleRoomLoadCompleted(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
	const FString RequestedRoomSaveId = PendingLoadRoomSaveId;
	PendingLoadRoomSaveId.Reset();
	Operation = EOperation::None;

	UPDRoomSaveGame* LoadedRoomSaveGame = Cast<UPDRoomSaveGame>(LoadedGameData);
	if (!LoadedRoomSaveGame)
	{
		const FString Error = TEXT("The room save file is missing, corrupted, or has an unexpected class.");
		UE_LOG(LogPDServer, Error, TEXT("Cannot load room save. RoomSaveId=%s Error=%s"), *RequestedRoomSaveId, *Error);
		OnRoomLoadCompleted.Broadcast(false, RequestedRoomSaveId, nullptr, Error);
		return;
	}

	FString ValidationError;
	if (!LoadedRoomSaveGame->Validate(ValidationError))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot load room save because validation failed. RoomSaveId=%s Error=%s"), *RequestedRoomSaveId, *ValidationError);
		OnRoomLoadCompleted.Broadcast(false, RequestedRoomSaveId, nullptr, ValidationError);
		return;
	}

	if (LoadedRoomSaveGame->RoomSaveId != RequestedRoomSaveId)
	{
		const FString Error = TEXT("The loaded room save ID does not match the requested room save ID.");
		UE_LOG(LogPDServer, Error, TEXT("Cannot load room save. RequestedRoomSaveId=%s LoadedRoomSaveId=%s"), *RequestedRoomSaveId, *LoadedRoomSaveGame->RoomSaveId);
		OnRoomLoadCompleted.Broadcast(false, RequestedRoomSaveId, nullptr, Error);
		return;
	}

	if (LoadedRoomSaveGame->HostPlatformUserId != ActiveHostPlatformUserId)
	{
		const FString Error = TEXT("The room save belongs to a different host platform user.");
		UE_LOG(LogPDServer, Warning, TEXT("Cannot load a room save owned by a different platform user. RoomSaveId=%s"), *RequestedRoomSaveId);
		OnRoomLoadCompleted.Broadcast(false, RequestedRoomSaveId, nullptr, Error);
		return;
	}

	ActiveRoomSaveId = LoadedRoomSaveGame->RoomSaveId;
	ActiveRoomDisplayName = LoadedRoomSaveGame->RoomDisplayName;
	ActiveRoomSaveGame = LoadedRoomSaveGame;
	UE_LOG(LogPDServer, Log, TEXT("Loaded room save for resume. RoomSaveId=%s"), *ActiveRoomSaveId);
	OnRoomLoadCompleted.Broadcast(true, ActiveRoomSaveId, LoadedRoomSaveGame, FString());
}

void UPDRoomSaveSubsystem::HandleRoomSaveCompleted(const FString& SlotName, const int32 UserIndex, const bool bSuccess)
{
	const FString SavedRoomSaveId = ActiveRoomSaveId;
	if (!bSuccess || !PendingRoomSaveGame)
	{
		const FString Error = TEXT("The room save file could not be written.");
		PendingRoomSaveGame = nullptr;
		Operation = EOperation::None;
		UE_LOG(LogPDServer, Error, TEXT("Room save failed. RoomSaveId=%s"), *SavedRoomSaveId);
		OnRoomSaveCompleted.Broadcast(false, SavedRoomSaveId, Error);
		StartQueuedSaveIfNeeded();
		return;
	}

	ActiveRoomSaveGame = PendingRoomSaveGame;
	PendingRoomSaveGame = nullptr;

	FString IndexLoadError;
	UPDRoomSaveIndexSaveGame* IndexSaveGame = LoadIndexSaveGame(IndexLoadError);
	if (!IndexSaveGame)
	{
		Operation = EOperation::None;
		UE_LOG(LogPDServer, Error, TEXT("Room save succeeded but the save index could not be loaded. RoomSaveId=%s Error=%s"), *SavedRoomSaveId, *IndexLoadError);
		OnRoomSaveCompleted.Broadcast(false, SavedRoomSaveId, IndexLoadError);
		StartQueuedSaveIfNeeded();
		return;
	}

	IndexSaveGame->UpsertListEntry(ActiveRoomSaveGame->CreateListEntry());
	PendingIndexSaveGame = IndexSaveGame;
	PendingIndexRoomSaveId = SavedRoomSaveId;
	Operation = EOperation::SavingIndex;
	UGameplayStatics::AsyncSaveGameToSlot(
		IndexSaveGame,
		GetIndexSaveSlotName(),
		SaveUserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &ThisClass::HandleIndexSaveCompleted));
}

void UPDRoomSaveSubsystem::HandleIndexSaveCompleted(const FString& SlotName, const int32 UserIndex, const bool bSuccess)
{
	const FString SavedRoomSaveId = PendingIndexRoomSaveId;
	UPDRoomSaveIndexSaveGame* SavedIndexSaveGame = PendingIndexSaveGame;
	PendingIndexRoomSaveId.Reset();
	PendingIndexSaveGame = nullptr;
	Operation = EOperation::None;

	if (!bSuccess || !SavedIndexSaveGame)
	{
		const FString Error = TEXT("The room save index could not be written.");
		UE_LOG(LogPDServer, Error, TEXT("Room save index update failed. RoomSaveId=%s"), *SavedRoomSaveId);
		OnRoomSaveCompleted.Broadcast(false, SavedRoomSaveId, Error);
		StartQueuedSaveIfNeeded();
		return;
	}

	UpdateCachedListEntries(*SavedIndexSaveGame, ActiveHostPlatformUserId);
	UE_LOG(LogPDServer, Log, TEXT("Room save completed. RoomSaveId=%s"), *SavedRoomSaveId);
	OnRoomSaveCompleted.Broadcast(true, SavedRoomSaveId, FString());
	StartQueuedSaveIfNeeded();
}
