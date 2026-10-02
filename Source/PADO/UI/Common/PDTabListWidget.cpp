// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Common/PDTabListWidget.h"

#include "Blueprint/UserWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "InputAction.h"
#include "PADO/UI/Common/PDButtonBase.h"
#include "PADO/UI/PDUILog.h"

bool UPDTabListWidget::AddTab(const FName TabId, const FText& Label, UWidget* ContentWidget)
{
	if (!TabButtonClass)
	{
		UE_LOG(LogPDUI, Warning, TEXT("%s has no TabButtonClass. Assign it in the widget defaults."), *GetNameSafe(GetClass()));
		return false;
	}

	if (!RegisterTab(TabId, TabButtonClass, ContentWidget))
	{
		return false;
	}

	if (UPDButtonBase* TabButton = Cast<UPDButtonBase>(GetTabButtonBaseByID(TabId)))
	{
		TabButton->SetButtonText(Label);
	}

	return true;
}

#if WITH_EDITOR
void UPDTabListWidget::ShowDesignerPreview(const TArray<FText>& Labels, const int32 SelectedIndex)
{
	bHasOwnerPreview = true;
	BuildDesignerPreview(Labels, SelectedIndex);
}
#endif

void UPDTabListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

#if WITH_EDITOR
	// 이 WBP만 디자이너에서 열었을 때 버튼 WBP의 기본 문구로 탭 몇 개를 그린다.
	if (IsDesignTime() && !bHasOwnerPreview)
	{
		TArray<FText> Labels;
		Labels.SetNum(DesignerPreviewTabCount);
		BuildDesignerPreview(Labels, 0);
	}
#endif
}

void UPDTabListWidget::UpdateBindings()
{
	// 이전/다음 탭 입력을 지정하지 않은 탭 목록은 빈 입력을 등록하면서 오류 로그를 남기므로 건너뛴다.
	const bool bHasTabInput = !NextTabInputActionData.IsNull() || !PreviousTabInputActionData.IsNull()
		|| NextTabEnhancedInputAction || PreviousTabEnhancedInputAction;
	if (bHasTabInput)
	{
		Super::UpdateBindings();
	}
}

void UPDTabListWidget::HandleTabCreation_Implementation(const FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	if (SeparatorClass && HBox_Tabs->HasAnyChildren())
	{
		if (UUserWidget* Separator = CreateWidget<UUserWidget>(this, SeparatorClass))
		{
			AddToTabRow(Separator);
			SeparatorsByTab.Add(TabNameID, Separator);
		}
	}

	AddToTabRow(TabButton);
}

void UPDTabListWidget::HandleTabRemoval_Implementation(const FName TabNameID, UCommonButtonBase* TabButton)
{
	if (TabButton)
	{
		TabButton->RemoveFromParent();
	}

	TObjectPtr<UUserWidget> Separator;
	if (SeparatorsByTab.RemoveAndCopyValue(TabNameID, Separator) && Separator)
	{
		Separator->RemoveFromParent();
	}

	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);
}

void UPDTabListWidget::AddToTabRow(UWidget* Widget)
{
	if (UHorizontalBoxSlot* TabSlot = HBox_Tabs->AddChildToHorizontalBox(Widget))
	{
		TabSlot->SetVerticalAlignment(VAlign_Center);
	}
}

#if WITH_EDITOR
void UPDTabListWidget::BuildDesignerPreview(const TArray<FText>& Labels, const int32 SelectedIndex)
{
	HBox_Tabs->ClearChildren();
	if (!TabButtonClass)
	{
		return;
	}

	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		if (Index > 0 && SeparatorClass)
		{
			AddToTabRow(CreateWidget<UUserWidget>(this, SeparatorClass));
		}

		UPDButtonBase* TabButton = CreateWidget<UPDButtonBase>(this, TabButtonClass);
		if (!Labels[Index].IsEmpty())
		{
			TabButton->SetButtonText(Labels[Index]);
		}

		// 선택된 탭 스타일도 확인할 수 있게 한 탭을 선택 상태로 그린다.
		TabButton->SetIsSelectable(true);
		TabButton->SetIsSelected(Index == SelectedIndex, false);
		AddToTabRow(TabButton);
	}
}
#endif
