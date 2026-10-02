// Copyright PADO. All Rights Reserved.

#include "PADO/Save/Struct/PDRoomSaveListEntry.h"

bool FPDRoomSaveListEntry::Validate(FString& OutError) const
{
	OutError.Reset();

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

	if (CompletedGeneralDeliveryCount < 0)
	{
		OutError = TEXT("CompletedGeneralDeliveryCount cannot be negative.");
		return false;
	}

	return true;
}
