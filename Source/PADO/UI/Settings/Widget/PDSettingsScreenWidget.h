// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FieldNotificationId.h"
#include "InputCoreTypes.h"
#include "PADO/UI/Common/PDActivatableWidget.h"
#include "PADO/UI/Common/Struct/PDConfirmDialogArgs.h"
#include "PADO/UI/Settings/Enum/PDSettingsTab.h"
#include "PDSettingsScreenWidget.generated.h"

class UCommonActivatableWidgetStack;
class UCommonAnimatedSwitcher;
class UPDButtonBase;
class UPDConfirmDialogWidget;
class UPDKeyCaptureWidget;
class UPDSettingKeyBindingViewModel;
class UPDSettingsPanelWidget;
class UPDSettingsViewModel;
class UPDTabListWidget;

/**
 * 옵션 창이다. 메인 메뉴와 인게임 메뉴 어디서든 Activatable Widget Stack에 올려 쓴다.
 * 탭마다 옵션 줄 패널을 하나씩 만들고, 적용을 누를 때만 설정을 반영·저장한다.
 * 취소·닫기·뒤로가기(Esc)는 적용하지 않은 변경이 있으면 확인을 받은 뒤 버리고 닫는다.
 * 확인창과 키 입력 대기 창은 창 안의 Stack_Dialogs에 올려 옵션 창 위에 띄운다.
 * 디자이너에서는 실제 탭 이름과 DesignerPreviewTab의 기본값 줄을 예시로 그린다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingsScreenWidget : public UPDActivatableWidget
{
	GENERATED_BODY()

public:
	UPDSettingsScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** 탭 하나의 옵션 줄을 담을 패널 WBP다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDSettingsPanelWidget> PanelClass;

	/** 닫기·키 중복 확인에 쓸 공용 확인창 WBP다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDConfirmDialogWidget> ConfirmDialogClass;

	/** 키 설정에서 새 키를 기다리는 창 WBP다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDKeyCaptureWidget> KeyCaptureClass;

#if WITH_EDITORONLY_DATA
	/** 디자이너에서 예시로 펼쳐 둘 탭이다. 실행 중에는 쓰지 않는다. */
	UPROPERTY(EditAnywhere, Category = "PADO|Settings|Designer")
	EPDSettingsTab DesignerPreviewTab = EPDSettingsTab::Display;
#endif

private:
	void RegisterTabs();

#if WITH_EDITOR
	void BuildDesignerPreview();
#endif
	void HandleApplyClicked();
	void HandleDefaultsClicked();
	void HandleViewModelFieldChanged(UObject* ViewModel, UE::FieldNotification::FFieldId FieldId);
	void RefreshButtons();

	/** 적용하지 않은 변경이 있으면 확인을 받은 뒤 닫는다. */
	void RequestClose();
	void CloseScreen();

	/** 공용 확인창을 띄운다. 확인창 WBP가 없으면 확인한 것으로 보고 바로 결과를 넘긴다. */
	void ShowConfirmDialog(FPDConfirmDialogArgs Args);

	void HandleKeyCaptureRequested(UPDSettingKeyBindingViewModel* Item);
	void HandleKeyCaptured(FName MappingName, const FKey& Key);

	EPDSettingsTab GetActiveTab() const;

	UFUNCTION()
	void HandleTabSelected(FName TabId);

	static FName GetTabId(EPDSettingsTab Tab);
	static const TCHAR* GetTabLabelKey(EPDSettingsTab Tab);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDTabListWidget> TabList_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonAnimatedSwitcher> Switcher_Panels;

	/** 확인창과 키 입력 대기 창을 옵션 창 위에 올리는 스택이다. 다른 위젯보다 위에 둔다. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> Stack_Dialogs;

	/** 선택 요소다. 오른쪽 위 닫기(X) 버튼이며 취소와 같게 동작한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPDButtonBase> Button_Close;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Defaults;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Cancel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPDButtonBase> Button_Apply;

	UPROPERTY(Transient)
	TObjectPtr<UPDSettingsViewModel> SettingsViewModel;

	UPROPERTY(Transient)
	TMap<EPDSettingsTab, TObjectPtr<UPDSettingsPanelWidget>> Panels;

#if WITH_EDITORONLY_DATA
	/** 디자이너 예시 줄이 쓰는 옵션 상태다. 줄이 이 객체의 값을 읽으므로 잡아 둔다. */
	UPROPERTY(Transient)
	TObjectPtr<UPDSettingsViewModel> DesignerPreviewViewModel;
#endif
};
