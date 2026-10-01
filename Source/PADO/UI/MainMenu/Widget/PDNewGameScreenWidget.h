// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FieldNotificationId.h"
#include "PADO/UI/MainMenu/Widget/PDMainMenuScreenBase.h"
#include "PDNewGameScreenWidget.generated.h"

class UEditableTextBox;
class UPDButtonBase;
class UPDNewGameViewModel;

/**
 * 새 게임 화면이다. 방 이름과 공개/친구(초대 전용)를 입력받아 방을 연다.
 * 문구·상태 표시는 WBP의 MVVM 바인딩이, 버튼 사용 가능 여부는 이 클래스가 Viewmodel을 따라 갱신한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDNewGameScreenWidget : public UPDMainMenuScreenBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	UFUNCTION()
	void HandleRoomNameChanged(const FText& Text);

	void HandlePublicClicked();
	void HandleFriendsOnlyClicked();
	void HandleCreateClicked();
	void HandleViewModelFieldChanged(UObject* ViewModel, UE::FieldNotification::FFieldId FieldId);
	void RefreshButtons();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TextBox_RoomName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Public;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_FriendsOnly;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Create;

	UPROPERTY(Transient)
	TObjectPtr<UPDNewGameViewModel> NewGameViewModel;
};
