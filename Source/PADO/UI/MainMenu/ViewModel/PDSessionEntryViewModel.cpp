// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/ViewModel/PDSessionEntryViewModel.h"

#include "PADO/UI/MainMenu/PDMainMenuText.h"
#include "ReusableSessionTypes.h"

void UPDSessionEntryViewModel::InitializeFromSession(const FReusableSessionEntry& InEntry)
{
	const FString TrimmedRoomName = InEntry.RoomName.TrimStartAndEnd();
	UE_MVVM_SET_PROPERTY_VALUE(RoomName, TrimmedRoomName.IsEmpty()
		? PDMainMenuText::Get(PDMainMenuText::Key::SessionUnnamedRoom)
		: FText::AsCultureInvariant(TrimmedRoomName));
	UE_MVVM_SET_PROPERTY_VALUE(HostName, FText::AsCultureInvariant(InEntry.HostName));

	FFormatNamedArguments PlayerCountArgs;
	PlayerCountArgs.Add(PDMainMenuText::Arg::Current, InEntry.CurrentPlayers);
	PlayerCountArgs.Add(PDMainMenuText::Arg::Max, InEntry.MaxPlayers);
	UE_MVVM_SET_PROPERTY_VALUE(PlayerCountText, FText::Format(PDMainMenuText::Get(PDMainMenuText::Key::SessionPlayerCount), PlayerCountArgs));

	FFormatNamedArguments PingArgs;
	PingArgs.Add(PDMainMenuText::Arg::Ping, InEntry.PingInMs);
	UE_MVVM_SET_PROPERTY_VALUE(PingText, FText::Format(PDMainMenuText::Get(PDMainMenuText::Key::SessionPing), PingArgs));

	UE_MVVM_SET_PROPERTY_VALUE(bIsFull, InEntry.MaxPlayers > 0 && InEntry.CurrentPlayers >= InEntry.MaxPlayers);
	SessionResult = InEntry.SessionResult;
}
