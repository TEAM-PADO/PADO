// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDNewGameScreenWidget.h"

#include "Components/EditableTextBox.h"
#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/Common/PDViewModelUtils.h"
#include "PADO/UI/MainMenu/PDMainMenuText.h"
#include "PADO/UI/MainMenu/ViewModel/PDNewGameViewModel.h"

void UPDNewGameScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	NewGameViewModel = NewObject<UPDNewGameViewModel>(this);
	PDViewModelUtils::SetViewModel(this, NewGameViewModel);

	TextBox_RoomName->OnTextChanged.AddDynamic(this, &ThisClass::HandleRoomNameChanged);
	Button_Public->OnClicked().AddUObject(this, &ThisClass::HandlePublicClicked);
	Button_FriendsOnly->OnClicked().AddUObject(this, &ThisClass::HandleFriendsOnlyClicked);
	Button_Create->OnClicked().AddUObject(this, &ThisClass::HandleCreateClicked);

	const INotifyFieldValueChanged::FFieldValueChangedDelegate FieldChangedDelegate =
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleViewModelFieldChanged);
	NewGameViewModel->AddFieldValueChangedDelegate(UPDNewGameViewModel::FFieldNotificationClassDescriptor::CanCreate, FieldChangedDelegate);
	NewGameViewModel->AddFieldValueChangedDelegate(UPDNewGameViewModel::FFieldNotificationClassDescriptor::CanSelectFriendsOnly, FieldChangedDelegate);
	NewGameViewModel->AddFieldValueChangedDelegate(UPDNewGameViewModel::FFieldNotificationClassDescriptor::bIsFriendsOnly, FieldChangedDelegate);
	NewGameViewModel->AddFieldValueChangedDelegate(UPDNewGameViewModel::FFieldNotificationClassDescriptor::bIsBusy, FieldChangedDelegate);

	Button_Public->SetIsSelectable(true);
	Button_FriendsOnly->SetIsSelectable(true);
}

void UPDNewGameScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	NewGameViewModel->Initialize(GetGameInstance());
	RefreshButtons();
}

void UPDNewGameScreenWidget::NativeDestruct()
{
	NewGameViewModel->Deinitialize();

	Super::NativeDestruct();
}

UWidget* UPDNewGameScreenWidget::NativeGetDesiredFocusTarget() const
{
	return TextBox_RoomName;
}

void UPDNewGameScreenWidget::HandleRoomNameChanged(const FText& Text)
{
	NewGameViewModel->SetRoomName(Text);
}

void UPDNewGameScreenWidget::HandlePublicClicked()
{
	NewGameViewModel->SetIsFriendsOnly(false);
	RefreshButtons();
}

void UPDNewGameScreenWidget::HandleFriendsOnlyClicked()
{
	NewGameViewModel->SetIsFriendsOnly(true);
	RefreshButtons();
}

void UPDNewGameScreenWidget::HandleCreateClicked()
{
	NewGameViewModel->CreateGame();
}

void UPDNewGameScreenWidget::HandleViewModelFieldChanged(UObject* ViewModel, UE::FieldNotification::FFieldId FieldId)
{
	RefreshButtons();
}

void UPDNewGameScreenWidget::RefreshButtons()
{
	const bool bIsBusy = NewGameViewModel->IsBusy();
	const bool bIsFriendsOnly = NewGameViewModel->IsFriendsOnly();

	// 선택 표시는 클릭 피드백 없이 Viewmodel 상태로만 맞춘다.
	Button_Public->SetIsSelected(!bIsFriendsOnly, false);
	Button_FriendsOnly->SetIsSelected(bIsFriendsOnly, false);

	Button_Public->SetIsEnabled(!bIsBusy);
	if (UPDNewGameViewModel::bFriendsOnlySessionSupported)
	{
		Button_FriendsOnly->SetIsEnabled(NewGameViewModel->CanSelectFriendsOnly());
	}
	else
	{
		Button_FriendsOnly->DisableButtonWithReason(PDMainMenuText::Get(PDMainMenuText::Key::NewGameFriendsOnlyUnavailable));
	}

	Button_Create->SetIsEnabled(NewGameViewModel->CanCreate());
	TextBox_RoomName->SetIsEnabled(!bIsBusy);
}
