// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/ViewModel/PDNewGameViewModel.h"

#include "Engine/GameInstance.h"
#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"
#include "PADO/UI/MainMenu/PDMainMenuText.h"
#include "PADO/UI/PDUILog.h"
#include "ReusableSteamSessionSubsystem.h"

void UPDNewGameViewModel::Initialize(UGameInstance* InGameInstance)
{
	Deinitialize();

	GameInstance = InGameInstance;
	UReusableSteamSessionSubsystem* Subsystem = UPDSteamSessionBlueprintLibrary::GetSteamSessionSubsystem(InGameInstance);
	SessionSubsystem = Subsystem;
	if (!Subsystem)
	{
		UE_LOG(LogPDUI, Warning, TEXT("New game view model could not find the Steam session subsystem."));
		return;
	}

	Subsystem->OnCreateComplete.AddUniqueDynamic(this, &ThisClass::HandleCreateComplete);
}

void UPDNewGameViewModel::Deinitialize()
{
	if (UReusableSteamSessionSubsystem* Subsystem = SessionSubsystem.Get())
	{
		Subsystem->OnCreateComplete.RemoveDynamic(this, &ThisClass::HandleCreateComplete);
	}

	SessionSubsystem.Reset();
	GameInstance.Reset();
	bIsCreatePending = false;
	SetBusy(false);
}

void UPDNewGameViewModel::SetRoomName(const FText& InRoomName)
{
	if (!UE_MVVM_SET_PROPERTY_VALUE(RoomName, InRoomName))
	{
		return;
	}

	if (GetTrimmedRoomName().Len() > MaxRoomNameLength)
	{
		FFormatNamedArguments Args;
		Args.Add(PDMainMenuText::Arg::Max, MaxRoomNameLength);
		SetStatusText(FText::Format(PDMainMenuText::Get(PDMainMenuText::Key::NewGameRoomNameTooLong), Args));
	}
	else
	{
		SetStatusText(FText::GetEmpty());
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanCreate);
}

void UPDNewGameViewModel::SetIsFriendsOnly(const bool bInIsFriendsOnly)
{
	if (bInIsFriendsOnly && !CanSelectFriendsOnly())
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::NewGameFriendsOnlyUnavailable));
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(bIsFriendsOnly, bInIsFriendsOnly);
}

void UPDNewGameViewModel::CreateGame()
{
	if (!CanCreate())
	{
		return;
	}

	UReusableSteamSessionSubsystem* Subsystem = SessionSubsystem.Get();
	if (!Subsystem)
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::NewGameCreateFailed));
		return;
	}

	if (Subsystem->IsOperationInProgress())
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::CommonBusy));
		return;
	}

	SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::NewGameCreating));
	SetBusy(true);

	// 플러그인은 즉시 실패도 완료 이벤트로 알리므로 요청 전에 대기 상태를 기록한다.
	// 친구(초대 전용)는 bFriendsOnlySessionSupported가 true가 된 뒤 이 호출에 전달한다.
	bIsCreatePending = true;
	if (!UPDSteamSessionBlueprintLibrary::CreatePADOListenSession(GameInstance.Get(), GetTrimmedRoomName()))
	{
		bIsCreatePending = false;
		SetBusy(false);
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::NewGameCreateFailed));
	}
}

bool UPDNewGameViewModel::CanCreate() const
{
	return !bIsBusy && IsRoomNameValid();
}

bool UPDNewGameViewModel::CanSelectFriendsOnly() const
{
	return bFriendsOnlySessionSupported && !bIsBusy;
}

void UPDNewGameViewModel::HandleCreateComplete(const bool bSuccess, const FString& Error)
{
	if (!bIsCreatePending)
	{
		return;
	}

	bIsCreatePending = false;
	if (bSuccess)
	{
		// 생성 성공 시 플러그인이 ListenServerMap으로 이동한다. 이동 전까지 입력을 잠가 둔다.
		return;
	}

	UE_LOG(LogPDUI, Warning, TEXT("Create session failed: %s"), *Error);
	SetBusy(false);
	SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::NewGameCreateFailed));
}

FString UPDNewGameViewModel::GetTrimmedRoomName() const
{
	return RoomName.ToString().TrimStartAndEnd();
}

bool UPDNewGameViewModel::IsRoomNameValid() const
{
	const FString TrimmedRoomName = GetTrimmedRoomName();
	return !TrimmedRoomName.IsEmpty() && TrimmedRoomName.Len() <= MaxRoomNameLength;
}

void UPDNewGameViewModel::SetBusy(const bool bInIsBusy)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, bInIsBusy))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanCreate);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanSelectFriendsOnly);
	}
}

void UPDNewGameViewModel::SetStatusText(const FText& InStatusText)
{
	UE_MVVM_SET_PROPERTY_VALUE(StatusText, InStatusText);
}
