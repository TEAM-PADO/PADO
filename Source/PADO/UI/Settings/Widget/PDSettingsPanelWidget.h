// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PADO/UI/Settings/Enum/PDSettingsTab.h"
#include "PDSettingsPanelWidget.generated.h"

class UPanelWidget;
class UPDSettingChoiceRowWidget;
class UPDSettingItemViewModel;
class UPDSettingKeyBindingRowWidget;
class UPDSettingRowWidget;
class UPDSettingSliderRowWidget;
class UPDSettingsViewModel;
class UPDSettingToggleRowWidget;

/**
 * 옵션 창 탭 하나의 내용이다. 항목 종류에 맞는 줄 WBP를 만들어 위에서 아래로 늘어놓는다.
 * Panel_Rows는 항목이 늘어날 때를 대비해 Scroll Box를 권장한다.
 * 이 WBP만 디자이너에서 열면 DesignerPreviewTab의 기본값 줄을 예시로 그린다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingsPanelWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 기존 줄을 지우고 항목마다 종류에 맞는 줄 위젯을 하나씩 만든다. */
	void SetItems(const TArray<UPDSettingItemViewModel*>& Items);

	/** 편집할 수 있는 첫 줄의 조작 위젯이다. 탭을 열 때 초점을 줄 대상이다. */
	UWidget* GetFirstFocusTarget() const;

protected:
	virtual void NativePreConstruct() override;

	/** 줄마다 바깥에 두는 여백이다. 줄 사이 간격을 여기서 정한다. */
	UPROPERTY(EditAnywhere, Category = "PADO|Settings")
	FMargin RowPadding;

#if WITH_EDITORONLY_DATA
	/** 이 WBP를 디자이너에서 열었을 때 예시로 그릴 탭이다. 실행 중에는 쓰지 않는다. */
	UPROPERTY(EditAnywhere, Category = "PADO|Settings|Designer")
	EPDSettingsTab DesignerPreviewTab = EPDSettingsTab::Display;
#endif

	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDSettingChoiceRowWidget> ChoiceRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDSettingSliderRowWidget> SliderRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDSettingToggleRowWidget> ToggleRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDSettingKeyBindingRowWidget> KeyBindingRowClass;

	/** 소제목·안내 줄이다. Text_Label만 있는 WBP를 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|Settings")
	TSubclassOf<UPDSettingRowWidget> HeaderRowClass;

private:
	void BuildRows(const TArray<UPDSettingItemViewModel*>& Items);
	TSubclassOf<UPDSettingRowWidget> GetRowClass(const UPDSettingItemViewModel& Item) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> Panel_Rows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDSettingRowWidget>> Rows;

	/** 옵션 창이 항목을 넘겼는지다. 넘겼으면 디자이너 예시로 덮어쓰지 않는다. */
	bool bHasAssignedItems = false;

#if WITH_EDITORONLY_DATA
	/** 디자이너 예시 줄이 쓰는 옵션 상태다. 줄이 이 객체의 값을 읽으므로 잡아 둔다. */
	UPROPERTY(Transient)
	TObjectPtr<UPDSettingsViewModel> DesignerPreviewViewModel;
#endif
};
