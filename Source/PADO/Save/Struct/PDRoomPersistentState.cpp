// Copyright PADO. All Rights Reserved.

#include "PADO/Save/Struct/PDRoomPersistentState.h"

bool FPDRoomPersistentState::Validate(FString& OutError) const
{
	OutError.Reset();

	if (CompletedGeneralDeliveryCount < 0)
	{
		OutError = TEXT("CompletedGeneralDeliveryCount cannot be negative.");
		return false;
	}

	return true;
}
