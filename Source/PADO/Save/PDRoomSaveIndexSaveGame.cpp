// Copyright PADO. All Rights Reserved.

#include "PADO/Save/PDRoomSaveIndexSaveGame.h"

int32 UPDRoomSaveIndexSaveGame::FindListEntryIndex(const FString& RoomSaveId) const
{
	return RoomSaveListEntries.IndexOfByPredicate([&RoomSaveId](const FPDRoomSaveListEntry& ListEntry)
	{
		return ListEntry.RoomSaveId == RoomSaveId;
	});
}

void UPDRoomSaveIndexSaveGame::UpsertListEntry(const FPDRoomSaveListEntry& ListEntry)
{
	const int32 ExistingIndex = FindListEntryIndex(ListEntry.RoomSaveId);
	if (ExistingIndex == INDEX_NONE)
	{
		RoomSaveListEntries.Add(ListEntry);
		return;
	}

	RoomSaveListEntries[ExistingIndex] = ListEntry;
}

bool UPDRoomSaveIndexSaveGame::RemoveListEntry(const FString& RoomSaveId)
{
	const int32 ExistingIndex = FindListEntryIndex(RoomSaveId);
	if (ExistingIndex == INDEX_NONE)
	{
		return false;
	}

	RoomSaveListEntries.RemoveAt(ExistingIndex);
	return true;
}

bool UPDRoomSaveIndexSaveGame::Validate(FString& OutError) const
{
	OutError.Reset();
	TSet<FString> SeenRoomSaveIds;

	for (const FPDRoomSaveListEntry& ListEntry : RoomSaveListEntries)
	{
		FString ListEntryError;
		if (!ListEntry.Validate(ListEntryError))
		{
			OutError = FString::Printf(TEXT("A room save list entry is invalid: %s"), *ListEntryError);
			return false;
		}

		if (SeenRoomSaveIds.Contains(ListEntry.RoomSaveId))
		{
			OutError = FString::Printf(TEXT("Duplicate RoomSaveId '%s' exists in the room save index."), *ListEntry.RoomSaveId);
			return false;
		}

		SeenRoomSaveIds.Add(ListEntry.RoomSaveId);
	}

	return true;
}
