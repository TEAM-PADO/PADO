// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/Widget/PDJoinGameScreenWidget.h"

#include "CommonListView.h"
#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/Common/PDViewModelUtils.h"
#include "PADO/UI/MainMenu/ViewModel/PDSessionBrowserViewModel.h"
#include "PADO/UI/MainMenu/ViewModel/PDSessionEntryViewModel.h"

void UPDJoinGameScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SessionBrowserViewModel = NewObject<UPDSessionBrowserViewModel>(this);
	PDViewModelUtils::SetViewModel(this, SessionBrowserViewModel);

	Button_Refresh->OnClicked().AddUObject(this, &ThisClass::HandleRefreshClicked);
	Button_Join->OnClicked().AddUObject(this, &ThisClass::HandleJoinClicked);
	ListView_Sessions->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
	ListView_Sessions->OnItemDoubleClicked().AddUObject(this, &ThisClass::HandleItemDoubleClicked);

	const INotifyFieldValueChanged::FFieldValueChangedDelegate FieldChangedDelegate =
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleViewModelFieldChanged);
	SessionBrowserViewModel->AddFieldValueChangedDelegate(UPDSessionBrowserViewModel::FFieldNotificationClassDescriptor::Entries, FieldChangedDelegate);
	SessionBrowserViewModel->AddFieldValueChangedDelegate(UPDSessionBrowserViewModel::FFieldNotificationClassDescriptor::CanRefresh, FieldChangedDelegate);
	SessionBrowserViewModel->AddFieldValueChangedDelegate(UPDSessionBrowserViewModel::FFieldNotificationClassDescriptor::CanJoin, FieldChangedDelegate);
}

void UPDJoinGameScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SessionBrowserViewModel->Initialize(GetGameInstance());
	RefreshButtons();
}

void UPDJoinGameScreenWidget::NativeDestruct()
{
	SessionBrowserViewModel->Deinitialize();

	Super::NativeDestruct();
}

void UPDJoinGameScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	SessionBrowserViewModel->RefreshSessions();
}

UWidget* UPDJoinGameScreenWidget::NativeGetDesiredFocusTarget() const
{
	return ListView_Sessions;
}

void UPDJoinGameScreenWidget::HandleRefreshClicked()
{
	SessionBrowserViewModel->RefreshSessions();
}

void UPDJoinGameScreenWidget::HandleJoinClicked()
{
	SessionBrowserViewModel->JoinSelectedSession();
}

void UPDJoinGameScreenWidget::HandleSelectionChanged(UObject* Item)
{
	SessionBrowserViewModel->SetSelectedEntry(Cast<UPDSessionEntryViewModel>(Item));
}

void UPDJoinGameScreenWidget::HandleItemDoubleClicked(UObject* Item)
{
	SessionBrowserViewModel->SetSelectedEntry(Cast<UPDSessionEntryViewModel>(Item));
	SessionBrowserViewModel->JoinSelectedSession();
}

void UPDJoinGameScreenWidget::HandleViewModelFieldChanged(UObject* ViewModel, const UE::FieldNotification::FFieldId FieldId)
{
	if (FieldId == UPDSessionBrowserViewModel::FFieldNotificationClassDescriptor::Entries)
	{
		RefreshList();
	}

	RefreshButtons();
}

void UPDJoinGameScreenWidget::RefreshList()
{
	TArray<UObject*> ListItems;
	ListItems.Reserve(SessionBrowserViewModel->GetEntries().Num());
	for (UPDSessionEntryViewModel* Entry : SessionBrowserViewModel->GetEntries())
	{
		ListItems.Add(Entry);
	}

	ListView_Sessions->ClearSelection();
	ListView_Sessions->SetListItems(ListItems);
}

void UPDJoinGameScreenWidget::RefreshButtons()
{
	Button_Refresh->SetIsEnabled(SessionBrowserViewModel->CanRefresh());
	Button_Join->SetIsEnabled(SessionBrowserViewModel->CanJoin());
}
