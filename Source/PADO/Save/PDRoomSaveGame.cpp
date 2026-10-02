// Copyright PADO. All Rights Reserved.

#include "PADO/Save/PDRoomSaveGame.h"

bool UPDRoomSaveGame::Validate(FString& OutError) const
{
	OutError.Reset();

	if (SaveSchemaVersion <= 0)
	{
		OutError = TEXT("SaveSchemaVersion must be greater than zero.");
		return false;
	}

	if (SaveSchemaVersion > CurrentSchemaVersion)
	{
		OutError = FString::Printf(TEXT("Save schema version %d is newer than the supported version %d."), SaveSchemaVersion, CurrentSchemaVersion);
		return false;
	}

	FGuid ParsedRoomSaveId;
	if (!FGuid::Parse(RoomSaveId, ParsedRoomSaveId) || !ParsedRoomSaveId.IsValid())
	{
		OutError = TEXT("RoomSaveId is missing or invalid.");
		return false;
	}

	if (HostPlatformUserId.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("HostPlatformUserId is empty.");
		return false;
	}

	return PersistentState.Validate(OutError);
}

FPDRoomSaveListEntry UPDRoomSaveGame::CreateListEntry() const
{
	FPDRoomSaveListEntry ListEntry;
	ListEntry.RoomSaveId = RoomSaveId;
	ListEntry.HostPlatformUserId = HostPlatformUserId;
	ListEntry.RoomDisplayName = RoomDisplayName;
	ListEntry.CompletedGeneralDeliveryCount = PersistentState.CompletedGeneralDeliveryCount;
	return ListEntry;
}
