// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/ViewModel/PDSessionBrowserViewModel.h"

#include "Engine/GameInstance.h"
#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"
#include "PADO/UI/MainMenu/PDMainMenuText.h"
#include "PADO/UI/MainMenu/ViewModel/PDSessionEntryViewModel.h"
#include "PADO/UI/PDUILog.h"
#include "ReusableSteamSessionSubsystem.h"

void UPDSessionBrowserViewModel::Initialize(UGameInstance* InGameInstance)
{
	Deinitialize();

	GameInstance = InGameInstance;
	UReusableSteamSessionSubsystem* Subsystem = UPDSteamSessionBlueprintLibrary::GetSteamSessionSubsystem(InGameInstance);
	SessionSubsystem = Subsystem;
	if (!Subsystem)
	{
		UE_LOG(LogPDUI, Warning, TEXT("Session browser could not find the Steam session subsystem."));
		return;
	}

	Subsystem->OnFindComplete.AddUniqueDynamic(this, &ThisClass::HandleFindComplete);
	Subsystem->OnJoinComplete.AddUniqueDynamic(this, &ThisClass::HandleJoinComplete);
}

void UPDSessionBrowserViewModel::Deinitialize()
{
	if (UReusableSteamSessionSubsystem* Subsystem = SessionSubsystem.Get())
	{
		Subsystem->OnFindComplete.RemoveDynamic(this, &ThisClass::HandleFindComplete);
		Subsystem->OnJoinComplete.RemoveDynamic(this, &ThisClass::HandleJoinComplete);
	}

	SessionSubsystem.Reset();
	GameInstance.Reset();
	bIsSearchPending = false;
	bIsJoinPending = false;
	SetBusy(false);
}

void UPDSessionBrowserViewModel::RefreshSessions()
{
	if (!CanRefresh())
	{
		return;
	}

	UReusableSteamSessionSubsystem* Subsystem = SessionSubsystem.Get();
	if (!Subsystem)
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameSearchFailed));
		return;
	}

	if (Subsystem->IsOperationInProgress())
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::CommonBusy));
		return;
	}

	SetSelectedEntry(nullptr);
	SetEntries({});
	UE_MVVM_SET_PROPERTY_VALUE(bHasNoResults, false);
	SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameSearching));
	SetBusy(true);

	// 플러그인은 즉시 실패도 완료 이벤트로 알리므로 요청 전에 대기 상태를 기록한다.
	bIsSearchPending = true;
	if (!UPDSteamSessionBlueprintLibrary::FindPADOSessions(GameInstance.Get()))
	{
		bIsSearchPending = false;
		SetBusy(false);
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameSearchFailed));
	}
}

void UPDSessionBrowserViewModel::SetSelectedEntry(UPDSessionEntryViewModel* InEntry)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(SelectedEntry, InEntry))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanJoin);
	}
}

void UPDSessionBrowserViewModel::JoinSelectedSession()
{
	if (!SelectedEntry || bIsBusy)
	{
		return;
	}

	if (SelectedEntry->IsFull())
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameRoomFull));
		return;
	}

	UReusableSteamSessionSubsystem* Subsystem = SessionSubsystem.Get();
	if (!Subsystem)
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameJoinFailed));
		return;
	}

	if (Subsystem->IsOperationInProgress())
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::CommonBusy));
		return;
	}

	SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameJoining));
	SetBusy(true);

	bIsJoinPending = true;
	if (!UPDSteamSessionBlueprintLibrary::JoinPADOSession(GameInstance.Get(), SelectedEntry->GetSessionResult()))
	{
		bIsJoinPending = false;
		SetBusy(false);
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameJoinFailed));
	}
}

bool UPDSessionBrowserViewModel::CanRefresh() const
{
	return !bIsBusy;
}

bool UPDSessionBrowserViewModel::CanJoin() const
{
	return !bIsBusy && SelectedEntry && !SelectedEntry->IsFull();
}

void UPDSessionBrowserViewModel::HandleFindComplete(const bool bSuccess, const TArray<FReusableSessionEntry>& Sessions)
{
	if (!bIsSearchPending)
	{
		return;
	}

	bIsSearchPending = false;
	SetBusy(false);

	if (!bSuccess)
	{
		SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameSearchFailed));
		return;
	}

	TArray<TObjectPtr<UPDSessionEntryViewModel>> NewEntries;
	NewEntries.Reserve(Sessions.Num());
	for (const FReusableSessionEntry& Session : Sessions)
	{
		// 친구(초대 전용) 방은 공개 목록에 표시하지 않는다.
		if (Session.bFriendsOnly)
		{
			continue;
		}

		UPDSessionEntryViewModel* Entry = NewObject<UPDSessionEntryViewModel>(this);
		Entry->InitializeFromSession(Session);
		NewEntries.Add(Entry);
	}

	SetEntries(NewEntries);
	UE_MVVM_SET_PROPERTY_VALUE(bHasNoResults, NewEntries.IsEmpty());
	SetStatusText(NewEntries.IsEmpty() ? PDMainMenuText::Get(PDMainMenuText::Key::JoinGameNoResults) : FText::GetEmpty());
}

void UPDSessionBrowserViewModel::HandleJoinComplete(const bool bSuccess, const FString& ConnectStringOrError)
{
	if (!bIsJoinPending)
	{
		return;
	}

	bIsJoinPending = false;
	if (bSuccess)
	{
		// 참가 성공 시 플러그인이 ClientTravel을 시작한다. 이동 전까지 입력을 잠가 둔다.
		return;
	}

	UE_LOG(LogPDUI, Warning, TEXT("Join session failed: %s"), *ConnectStringOrError);
	SetBusy(false);
	SetStatusText(PDMainMenuText::Get(PDMainMenuText::Key::JoinGameJoinFailed));
}

void UPDSessionBrowserViewModel::SetBusy(const bool bInIsBusy)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, bInIsBusy))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanRefresh);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanJoin);
	}
}

void UPDSessionBrowserViewModel::SetStatusText(const FText& InStatusText)
{
	UE_MVVM_SET_PROPERTY_VALUE(StatusText, InStatusText);
}

void UPDSessionBrowserViewModel::SetEntries(const TArray<TObjectPtr<UPDSessionEntryViewModel>>& InEntries)
{
	Entries = InEntries;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Entries);
}
