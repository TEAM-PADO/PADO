// Copyright PADO. All Rights Reserved.

#include "PADO/SteamSession/Struct/PDRoomAccessSettings.h"

bool FPDRoomAccessSettings::Validate(FString& OutError) const
{
	OutError.Reset();

	if (AccessPolicy != EPDRoomAccessPolicy::FriendsOrPassword)
	{
		return true;
	}

	return ValidatePassword(Password, OutError);
}

bool FPDRoomAccessSettings::ValidatePassword(const FString& Password, FString& OutError)
{
	OutError.Reset();

	constexpr int32 MinPasswordLength = 4;
	constexpr int32 MaxPasswordLength = 8;
	if (Password.Len() < MinPasswordLength || Password.Len() > MaxPasswordLength)
	{
		OutError = FString::Printf(TEXT("The room access code must contain %d to %d digits."), MinPasswordLength, MaxPasswordLength);
		return false;
	}

	for (const TCHAR Character : Password)
	{
		if (Character < TEXT('0') || Character > TEXT('9'))
		{
			OutError = TEXT("The room access code must contain ASCII digits only.");
			return false;
		}
	}

	return true;
}
