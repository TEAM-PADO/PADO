// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FieldNotificationId.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuScreenBase.h"
#include "PDJoinGameScreenWidget.generated.h"

class UCommonListView;
class UPDButtonBase;
class UPDSessionBrowserViewModel;

/**
 * 게임 참가 화면이다. 공개 방 목록을 조회하고 선택한 방에 참가한다.
 * 화면이 활성화될 때마다 목록을 새로 조회한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDJoinGameScreenWidget : public UPDMainMenuScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	void HandleRefreshClicked();
	void HandleJoinClicked();
	void HandleSelectionChanged(UObject* Item);
	void HandleItemDoubleClicked(UObject* Item);
	void HandleViewModelFieldChanged(UObject* ViewModel, UE::FieldNotification::FFieldId FieldId);
	void RefreshList();
	void RefreshButtons();

	/** 항목 위젯은 UPDSessionListEntryWidget을 부모로 한 WBP를 Entry Widget Class에 지정한다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonListView> ListView_Sessions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Refresh;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Join;

	UPROPERTY(Transient)
	TObjectPtr<UPDSessionBrowserViewModel> SessionBrowserViewModel;
};
