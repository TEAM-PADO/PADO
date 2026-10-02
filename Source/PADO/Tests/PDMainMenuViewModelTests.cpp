#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "PADO/UI/MainMenu/ViewModel/PDNewGameViewModel.h"
#include "PADO/UI/MainMenu/ViewModel/PDSessionBrowserViewModel.h"
#include "PADO/UI/MainMenu/ViewModel/PDSessionEntryViewModel.h"
#include "ReusableSessionTypes.h"
#include "UObject/Package.h"

namespace PDMainMenuViewModelTests
{
	FReusableSessionEntry MakeSessionEntry(const int32 CurrentPlayers, const int32 MaxPlayers)
	{
		FReusableSessionEntry Entry;
		Entry.RoomName = TEXT("Room");
		Entry.HostName = TEXT("Host");
		Entry.CurrentPlayers = CurrentPlayers;
		Entry.MaxPlayers = MaxPlayers;
		Entry.PingInMs = 30;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDNewGameViewModelValidationTest,
	"PADO.UI.MainMenu.NewGame.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDNewGameViewModelValidationTest::RunTest(const FString& Parameters)
{
	UPDNewGameViewModel* ViewModel = NewObject<UPDNewGameViewModel>(GetTransientPackage());

	TestFalse(TEXT("An empty room name cannot create a game."), ViewModel->CanCreate());

	ViewModel->SetRoomName(FText::FromString(TEXT("   ")));
	TestFalse(TEXT("A whitespace-only room name cannot create a game."), ViewModel->CanCreate());

	ViewModel->SetRoomName(FText::FromString(TEXT("  PADO Room  ")));
	TestTrue(TEXT("A trimmed room name can create a game."), ViewModel->CanCreate());

	ViewModel->SetRoomName(FText::FromString(FString::ChrN(UPDNewGameViewModel::MaxRoomNameLength, TEXT('A'))));
	TestTrue(TEXT("A room name at the length limit can create a game."), ViewModel->CanCreate());

	ViewModel->SetRoomName(FText::FromString(FString::ChrN(UPDNewGameViewModel::MaxRoomNameLength + 1, TEXT('A'))));
	TestFalse(TEXT("A room name over the length limit cannot create a game."), ViewModel->CanCreate());
	TestFalse(TEXT("A too-long room name shows a status message."), ViewModel->GetStatusText().IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDNewGameViewModelFriendsOnlyTest,
	"PADO.UI.MainMenu.NewGame.FriendsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDNewGameViewModelFriendsOnlyTest::RunTest(const FString& Parameters)
{
	UPDNewGameViewModel* ViewModel = NewObject<UPDNewGameViewModel>(GetTransientPackage());
	ViewModel->SetIsFriendsOnly(true);

	TestEqual(TEXT("Friends-only follows the supported flag."), ViewModel->IsFriendsOnly(), UPDNewGameViewModel::bFriendsOnlySessionSupported);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDNewGameViewModelNoSubsystemTest,
	"PADO.UI.MainMenu.NewGame.NoSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDNewGameViewModelNoSubsystemTest::RunTest(const FString& Parameters)
{
	UPDNewGameViewModel* ViewModel = NewObject<UPDNewGameViewModel>(GetTransientPackage());
	AddExpectedMessage(TEXT("could not find the Steam session subsystem"), EAutomationExpectedMessageFlags::Contains, 1);
	ViewModel->Initialize(nullptr);
	ViewModel->SetRoomName(FText::FromString(TEXT("Room")));
	ViewModel->CreateGame();

	TestFalse(TEXT("Creating without a session subsystem does not stay busy."), ViewModel->IsBusy());
	TestFalse(TEXT("Creating without a session subsystem shows a failure message."), ViewModel->GetStatusText().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSessionEntryViewModelTest,
	"PADO.UI.MainMenu.SessionEntry.Capacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSessionEntryViewModelTest::RunTest(const FString& Parameters)
{
	using namespace PDMainMenuViewModelTests;

	UPDSessionEntryViewModel* OpenEntry = NewObject<UPDSessionEntryViewModel>(GetTransientPackage());
	OpenEntry->InitializeFromSession(MakeSessionEntry(1, 4));
	TestFalse(TEXT("A room below capacity is not full."), OpenEntry->IsFull());
	TestEqual(TEXT("Room name is copied."), OpenEntry->GetRoomName().ToString(), FString(TEXT("Room")));

	UPDSessionEntryViewModel* FullEntry = NewObject<UPDSessionEntryViewModel>(GetTransientPackage());
	FullEntry->InitializeFromSession(MakeSessionEntry(4, 4));
	TestTrue(TEXT("A room at capacity is full."), FullEntry->IsFull());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSessionBrowserViewModelSelectionTest,
	"PADO.UI.MainMenu.SessionBrowser.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSessionBrowserViewModelSelectionTest::RunTest(const FString& Parameters)
{
	using namespace PDMainMenuViewModelTests;

	UPDSessionBrowserViewModel* ViewModel = NewObject<UPDSessionBrowserViewModel>(GetTransientPackage());
	TestFalse(TEXT("Nothing selected cannot join."), ViewModel->CanJoin());

	UPDSessionEntryViewModel* OpenEntry = NewObject<UPDSessionEntryViewModel>(GetTransientPackage());
	OpenEntry->InitializeFromSession(MakeSessionEntry(1, 4));
	ViewModel->SetSelectedEntry(OpenEntry);
	TestTrue(TEXT("An open room can be joined."), ViewModel->CanJoin());

	UPDSessionEntryViewModel* FullEntry = NewObject<UPDSessionEntryViewModel>(GetTransientPackage());
	FullEntry->InitializeFromSession(MakeSessionEntry(4, 4));
	ViewModel->SetSelectedEntry(FullEntry);
	TestFalse(TEXT("A full room cannot be joined."), ViewModel->CanJoin());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSessionBrowserViewModelNoSubsystemTest,
	"PADO.UI.MainMenu.SessionBrowser.NoSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSessionBrowserViewModelNoSubsystemTest::RunTest(const FString& Parameters)
{
	UPDSessionBrowserViewModel* ViewModel = NewObject<UPDSessionBrowserViewModel>(GetTransientPackage());
	AddExpectedMessage(TEXT("could not find the Steam session subsystem"), EAutomationExpectedMessageFlags::Contains, 1);
	ViewModel->Initialize(nullptr);
	ViewModel->RefreshSessions();

	TestFalse(TEXT("Refreshing without a session subsystem does not stay busy."), ViewModel->IsBusy());
	TestTrue(TEXT("Refreshing stays available after a failure."), ViewModel->CanRefresh());
	TestFalse(TEXT("Refreshing without a session subsystem shows a failure message."), ViewModel->GetStatusText().IsEmpty());
	return true;
}

#endif
