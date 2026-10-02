// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingsScreenWidget.h"

#include "CommonAnimatedSwitcher.h"
#include "Engine/LocalPlayer.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/Settings/PDKeyBindingSubsystem.h"
#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/Common/PDConfirmDialogWidget.h"
#include "PADO/UI/Common/PDTabListWidget.h"
#include "PADO/UI/PDUILog.h"
#include "PADO/UI/Settings/PDSettingsDesignerPreview.h"
#include "PADO/UI/Settings/PDSettingsText.h"
#include "PADO/UI/Settings/ViewModel/PDSettingKeyBindingViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingsViewModel.h"
#include "PADO/UI/Settings/Widget/PDKeyCaptureWidget.h"
#include "PADO/UI/Settings/Widget/PDSettingsPanelWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UPDSettingsScreenWidget::UPDSettingsScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
}

void UPDSettingsScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	SettingsViewModel = NewObject<UPDSettingsViewModel>(this);
	SettingsViewModel->Initialize(UPDGameUserSettings::Get(), LocalPlayer ? LocalPlayer->GetSubsystem<UPDKeyBindingSubsystem>() : nullptr);
	SettingsViewModel->OnKeyCaptureRequested.AddUObject(this, &ThisClass::HandleKeyCaptureRequested);

	TabList_Settings->SetLinkedSwitcher(Switcher_Panels);
	TabList_Settings->OnTabSelected.AddDynamic(this, &ThisClass::HandleTabSelected);

	if (PanelClass)
	{
		for (const EPDSettingsTab Tab : TEnumRange<EPDSettingsTab>())
		{
			UPDSettingsPanelWidget* Panel = CreateWidget<UPDSettingsPanelWidget>(this, PanelClass);
			Panel->SetItems(SettingsViewModel->GetItems(Tab));
			Switcher_Panels->AddChild(Panel);
			Panels.Add(Tab, Panel);
		}
	}
	else
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s has no PanelClass. Assign it in the widget defaults."), *GetNameSafe(GetClass()));
	}

	if (Button_Close)
	{
		Button_Close->OnClicked().AddUObject(this, &ThisClass::RequestClose);
	}
	Button_Defaults->OnClicked().AddUObject(this, &ThisClass::HandleDefaultsClicked);
	Button_Cancel->OnClicked().AddUObject(this, &ThisClass::RequestClose);
	Button_Apply->OnClicked().AddUObject(this, &ThisClass::HandleApplyClicked);

	const INotifyFieldValueChanged::FFieldValueChangedDelegate FieldChangedDelegate =
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleViewModelFieldChanged);
	SettingsViewModel->AddFieldValueChangedDelegate(UPDSettingsViewModel::FFieldNotificationClassDescriptor::bHasPendingChanges, FieldChangedDelegate);
	SettingsViewModel->AddFieldValueChangedDelegate(UPDSettingsViewModel::FFieldNotificationClassDescriptor::bIsAvailable, FieldChangedDelegate);
}

void UPDSettingsScreenWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

#if WITH_EDITOR
	if (IsDesignTime())
	{
		BuildDesignerPreview();
	}
#endif
}

void UPDSettingsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 탭 목록은 화면에서 내려갈 때 탭을 모두 지우므로 올라올 때마다 다시 등록한다.
	RegisterTabs();
}

void UPDSettingsScreenWidget::NativeDestruct()
{
	// 열린 채로 맵을 옮기는 경우에도 밝기·음량 미리보기가 남지 않게 한다.
	if (SettingsViewModel)
	{
		SettingsViewModel->Revert();
	}

	Super::NativeDestruct();
}

void UPDSettingsScreenWidget::NativeOnActivated()
{
	// 초점 대상이 현재 탭을 따르므로 활성화 처리 전에 값과 탭을 맞춘다.
	SettingsViewModel->ReloadValues();
	TabList_Settings->SelectTabByID(GetTabId(EPDSettingsTab::General), true);
	RefreshButtons();

	Super::NativeOnActivated();
}

bool UPDSettingsScreenWidget::NativeOnHandleBackAction()
{
	RequestClose();
	return true;
}

UWidget* UPDSettingsScreenWidget::NativeGetDesiredFocusTarget() const
{
	if (const TObjectPtr<UPDSettingsPanelWidget>* Panel = Panels.Find(GetActiveTab()))
	{
		if (UWidget* FocusTarget = (*Panel)->GetFirstFocusTarget())
		{
			return FocusTarget;
		}
	}

	return Button_Cancel;
}

void UPDSettingsScreenWidget::RegisterTabs()
{
	if (TabList_Settings->GetTabCount() > 0)
	{
		return;
	}

	for (const EPDSettingsTab Tab : TEnumRange<EPDSettingsTab>())
	{
		const TObjectPtr<UPDSettingsPanelWidget>* Panel = Panels.Find(Tab);
		TabList_Settings->AddTab(GetTabId(Tab), PDSettingsText::Get(GetTabLabelKey(Tab)), Panel ? Panel->Get() : nullptr);
	}
}

#if WITH_EDITOR
void UPDSettingsScreenWidget::BuildDesignerPreview()
{
	if (!DesignerPreviewViewModel)
	{
		DesignerPreviewViewModel = PDSettingsDesignerPreview::CreateViewModel(this);
	}

	TArray<FText> TabLabels;
	int32 PreviewTabIndex = 0;
	for (const EPDSettingsTab Tab : TEnumRange<EPDSettingsTab>())
	{
		if (Tab == DesignerPreviewTab)
		{
			PreviewTabIndex = TabLabels.Num();
		}
		TabLabels.Add(PDSettingsText::Get(GetTabLabelKey(Tab)));
	}
	TabList_Settings->ShowDesignerPreview(TabLabels, PreviewTabIndex);

	// 디자이너에서는 미리 볼 탭의 패널 하나만 만든다.
	Switcher_Panels->ClearChildren();
	if (PanelClass)
	{
		UPDSettingsPanelWidget* Panel = CreateWidget<UPDSettingsPanelWidget>(this, PanelClass);
		Panel->SetItems(DesignerPreviewViewModel->GetItems(DesignerPreviewTab));
		Switcher_Panels->AddChild(Panel);
	}
}
#endif

void UPDSettingsScreenWidget::HandleApplyClicked()
{
	SettingsViewModel->Apply();
}

void UPDSettingsScreenWidget::HandleDefaultsClicked()
{
	SettingsViewModel->ResetTabToDefaults(GetActiveTab());
}

void UPDSettingsScreenWidget::HandleViewModelFieldChanged(UObject* ViewModel, UE::FieldNotification::FFieldId FieldId)
{
	RefreshButtons();
}

void UPDSettingsScreenWidget::RefreshButtons()
{
	Button_Apply->SetIsEnabled(SettingsViewModel->HasPendingChanges());
	Button_Defaults->SetIsEnabled(SettingsViewModel->IsAvailable() || GetActiveTab() == EPDSettingsTab::KeyBindings);
}

void UPDSettingsScreenWidget::RequestClose()
{
	if (!SettingsViewModel->HasPendingChanges())
	{
		CloseScreen();
		return;
	}

	using namespace PDSettingsText::Key;
	FPDConfirmDialogArgs Args;
	Args.Title = PDSettingsText::Get(DialogUnsavedTitle);
	Args.Body = PDSettingsText::Get(DialogUnsavedBody);
	Args.ConfirmText = PDSettingsText::Get(DialogUnsavedConfirm);
	Args.CancelText = PDSettingsText::Get(DialogUnsavedCancel);
	Args.OnClosed = [WeakThis = TWeakObjectPtr<ThisClass>(this)](const EPDConfirmDialogResult Result)
	{
		if (ThisClass* Screen = WeakThis.Get(); Screen && Result == EPDConfirmDialogResult::Confirmed)
		{
			Screen->CloseScreen();
		}
	};
	ShowConfirmDialog(MoveTemp(Args));
}

void UPDSettingsScreenWidget::CloseScreen()
{
	SettingsViewModel->Revert();
	DeactivateWidget();
}

void UPDSettingsScreenWidget::ShowConfirmDialog(FPDConfirmDialogArgs Args)
{
	if (!ConfirmDialogClass)
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s has no ConfirmDialogClass. The dialog was skipped and treated as confirmed."), *GetNameSafe(GetClass()));
		if (Args.OnClosed)
		{
			Args.OnClosed(EPDConfirmDialogResult::Confirmed);
		}
		return;
	}

	Stack_Dialogs->AddWidget<UPDConfirmDialogWidget>(ConfirmDialogClass, [&Args](UPDConfirmDialogWidget& Dialog)
	{
		Dialog.Setup(MoveTemp(Args));
	});
}

void UPDSettingsScreenWidget::HandleKeyCaptureRequested(UPDSettingKeyBindingViewModel* Item)
{
	if (!Item)
	{
		return;
	}

	if (!KeyCaptureClass)
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s has no KeyCaptureClass. Assign it in the widget defaults."), *GetNameSafe(GetClass()));
		return;
	}

	const FName MappingName = Item->GetMappingName();
	const FText ActionName = Item->GetDisplayName();
	Stack_Dialogs->AddWidget<UPDKeyCaptureWidget>(KeyCaptureClass, [this, MappingName, &ActionName](UPDKeyCaptureWidget& KeyCapture)
	{
		KeyCapture.Setup(ActionName, [WeakThis = TWeakObjectPtr<ThisClass>(this), MappingName](const FKey& Key)
		{
			if (ThisClass* Screen = WeakThis.Get())
			{
				Screen->HandleKeyCaptured(MappingName, Key);
			}
		});
	});
}

void UPDSettingsScreenWidget::HandleKeyCaptured(const FName MappingName, const FKey& Key)
{
	const FName ConflictMappingName = SettingsViewModel->FindKeyConflict(MappingName, Key);
	if (ConflictMappingName.IsNone())
	{
		SettingsViewModel->AssignKey(MappingName, Key);
		return;
	}

	using namespace PDSettingsText::Key;
	FFormatNamedArguments BodyArgs;
	BodyArgs.Add(PDSettingsText::Arg::KeyName, Key.GetDisplayName(false));
	BodyArgs.Add(PDSettingsText::Arg::Action, SettingsViewModel->GetKeyBindingDisplayName(ConflictMappingName));

	FPDConfirmDialogArgs Args;
	Args.Title = PDSettingsText::Get(KeyBindingsConflictTitle);
	Args.Body = FText::Format(PDSettingsText::Get(KeyBindingsConflictBody), BodyArgs);
	Args.ConfirmText = PDSettingsText::Get(KeyBindingsConflictConfirm);
	Args.CancelText = PDSettingsText::Get(DialogCancel);
	Args.OnClosed = [WeakThis = TWeakObjectPtr<ThisClass>(this), MappingName, Key](const EPDConfirmDialogResult Result)
	{
		if (ThisClass* Screen = WeakThis.Get(); Screen && Result == EPDConfirmDialogResult::Confirmed)
		{
			Screen->SettingsViewModel->AssignKey(MappingName, Key);
		}
	};
	ShowConfirmDialog(MoveTemp(Args));
}

EPDSettingsTab UPDSettingsScreenWidget::GetActiveTab() const
{
	const FName ActiveTabId = TabList_Settings->GetActiveTab();
	for (const EPDSettingsTab Tab : TEnumRange<EPDSettingsTab>())
	{
		if (GetTabId(Tab) == ActiveTabId)
		{
			return Tab;
		}
	}

	return EPDSettingsTab::General;
}

void UPDSettingsScreenWidget::HandleTabSelected(FName TabId)
{
	RefreshButtons();

	// 탭을 넘기면 숨겨진 패널에 초점이 남지 않도록 새 패널의 첫 줄로 옮긴다.
	if (IsActivated())
	{
		RequestRefreshFocus();
	}
}

FName UPDSettingsScreenWidget::GetTabId(const EPDSettingsTab Tab)
{
	return FName(StaticEnum<EPDSettingsTab>()->GetNameStringByValue(static_cast<int64>(Tab)));
}

const TCHAR* UPDSettingsScreenWidget::GetTabLabelKey(const EPDSettingsTab Tab)
{
	using namespace PDSettingsText::Key;
	switch (Tab)
	{
	case EPDSettingsTab::General:
		return TabGeneral;

	case EPDSettingsTab::Display:
		return TabDisplay;

	case EPDSettingsTab::Audio:
		return TabAudio;

	case EPDSettingsTab::Controls:
		return TabControls;

	case EPDSettingsTab::KeyBindings:
		return TabKeyBindings;

	case EPDSettingsTab::Accessibility:
		return TabAccessibility;
	}

	return TabGeneral;
}
