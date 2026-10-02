// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/ViewModel/PDMainMenuViewModel.h"

#include "Engine/GameInstance.h"
#include "PADO/SteamSession/PDSteamSessionBlueprintLibrary.h"
#include "PADO/UI/MainMenu/PDMainMenuText.h"
#include "PADO/UI/PDUILog.h"
#include "ReusableSteamSessionSubsystem.h"

void UPDMainMenuViewModel::Initialize(UGameInstance* InGameInstance)
{
	Deinitialize();

	UReusableSteamSessionSubsystem* Subsystem = UPDSteamSessionBlueprintLibrary::GetSteamSessionSubsystem(InGameInstance);
	SessionSubsystem = Subsystem;
	if (!Subsystem)
	{
		UE_LOG(LogPDUI, Warning, TEXT("Main menu view model could not find the Steam session subsystem."));
		return;
	}

	Subsystem->OnInviteAccepted.AddUniqueDynamic(this, &ThisClass::HandleInviteAccepted);
	Subsystem->OnJoinComplete.AddUniqueDynamic(this, &ThisClass::HandleJoinComplete);
}

void UPDMainMenuViewModel::Deinitialize()
{
	if (UReusableSteamSessionSubsystem* Subsystem = SessionSubsystem.Get())
	{
		Subsystem->OnInviteAccepted.RemoveDynamic(this, &ThisClass::HandleInviteAccepted);
		Subsystem->OnJoinComplete.RemoveDynamic(this, &ThisClass::HandleJoinComplete);
	}

	SessionSubsystem.Reset();
	bIsInviteJoinPending = false;
	UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, false);
}

void UPDMainMenuViewModel::ClearNotice()
{
	UE_MVVM_SET_PROPERTY_VALUE(NoticeText, FText::GetEmpty());
}

void UPDMainMenuViewModel::HandleInviteAccepted(const FBlueprintSessionResult& Session)
{
	// 플러그인이 이 이벤트 직후 같은 결과로 JoinSession을 호출한다.
	bIsInviteJoinPending = true;
	UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, true);
	UE_MVVM_SET_PROPERTY_VALUE(NoticeText, PDMainMenuText::Get(PDMainMenuText::Key::InviteJoining));
}

void UPDMainMenuViewModel::HandleJoinComplete(const bool bSuccess, const FString& ConnectStringOrError)
{
	if (!bIsInviteJoinPending)
	{
		return;
	}

	bIsInviteJoinPending = false;
	if (bSuccess)
	{
		// 참가 성공 시 플러그인이 ClientTravel을 시작한다. 이동 전까지 입력을 잠가 둔다.
		return;
	}

	UE_LOG(LogPDUI, Warning, TEXT("Invite join failed: %s"), *ConnectStringOrError);
	UE_MVVM_SET_PROPERTY_VALUE(bIsBusy, false);
	UE_MVVM_SET_PROPERTY_VALUE(NoticeText, PDMainMenuText::Get(PDMainMenuText::Key::InviteJoinFailed));
}
