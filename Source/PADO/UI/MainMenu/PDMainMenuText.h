// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 메인 메뉴 String Table의 경로와 항목 키다.
 * C++에서 만드는 문구는 모두 이 키로 조회해 로컬라이징 대상에 포함한다.
 * 항목 원본은 Docs/UI/Localization/ST_MainMenu.csv이며, 에디터에서 String Table 에셋으로 가져온다.
 */
namespace PDMainMenuText
{
	/** 메인 메뉴 String Table 에셋 경로다. */
	inline constexpr const TCHAR* TableId = TEXT("/Game/PADO/UI/Localization/ST_MainMenu.ST_MainMenu");

	namespace Key
	{
		inline constexpr const TCHAR* CommonBusy = TEXT("Common.Busy");

		inline constexpr const TCHAR* NewGameRoomNameTooLong = TEXT("NewGame.RoomNameTooLong");
		inline constexpr const TCHAR* NewGameFriendsOnlyUnavailable = TEXT("NewGame.FriendsOnlyUnavailable");
		inline constexpr const TCHAR* NewGameCreating = TEXT("NewGame.Creating");
		inline constexpr const TCHAR* NewGameCreateFailed = TEXT("NewGame.CreateFailed");

		inline constexpr const TCHAR* JoinGameSearching = TEXT("JoinGame.Searching");
		inline constexpr const TCHAR* JoinGameSearchFailed = TEXT("JoinGame.SearchFailed");
		inline constexpr const TCHAR* JoinGameNoResults = TEXT("JoinGame.NoResults");
		inline constexpr const TCHAR* JoinGameJoining = TEXT("JoinGame.Joining");
		inline constexpr const TCHAR* JoinGameJoinFailed = TEXT("JoinGame.JoinFailed");
		inline constexpr const TCHAR* JoinGameRoomFull = TEXT("JoinGame.RoomFull");

		inline constexpr const TCHAR* SessionPlayerCount = TEXT("Session.PlayerCount");
		inline constexpr const TCHAR* SessionPing = TEXT("Session.Ping");
		inline constexpr const TCHAR* SessionUnnamedRoom = TEXT("Session.UnnamedRoom");

		inline constexpr const TCHAR* InviteJoining = TEXT("Invite.Joining");
		inline constexpr const TCHAR* InviteJoinFailed = TEXT("Invite.JoinFailed");
	}

	/** 형식 문구에 넘기는 인자 이름이다. String Table 원문의 {이름}과 같아야 한다. */
	namespace Arg
	{
		inline constexpr const TCHAR* Current = TEXT("Current");
		inline constexpr const TCHAR* Max = TEXT("Max");
		inline constexpr const TCHAR* Ping = TEXT("Ping");
	}

	/** 메인 메뉴 String Table 항목을 조회한다. */
	PADO_API FText Get(const TCHAR* InKey);
}
