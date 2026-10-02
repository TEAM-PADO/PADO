// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "PDTabListWidget.generated.h"

class UHorizontalBox;
class UPDButtonBase;

/**
 * PADO UI 공용 탭 목록이다. 탭 버튼을 가로로 늘어놓고 연결된 Switcher의 내용을 바꾼다.
 * 이전/다음 탭 입력(NextTabInputActionData 등)을 지정하면 키보드·게임패드로도 탭을 넘긴다.
 * 디자이너에서는 탭 버튼과 구분선을 예시로 그린다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	/** TabButtonClass로 탭 버튼을 만들어 문구를 지정하고 내용 위젯과 연결한다. */
	bool AddTab(FName TabId, const FText& Label, UWidget* ContentWidget);

#if WITH_EDITOR
	/**
	 * 디자이너에서 탭 버튼과 구분선을 예시로 그린다. 탭은 등록하지 않는다.
	 * 이 탭 목록을 쓰는 화면이 실제 탭 이름으로 부른다. 문구가 비어 있으면 버튼 WBP의 기본 문구를 쓴다.
	 */
	void ShowDesignerPreview(const TArray<FText>& Labels, int32 SelectedIndex);
#endif

protected:
	virtual void NativePreConstruct() override;
	virtual void UpdateBindings() override;
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

	/** 탭 버튼으로 쓸 WBP다. 선택 상태 스타일은 이 버튼의 Style에서 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|UI")
	TSubclassOf<UPDButtonBase> TabButtonClass;

	/** 선택 요소다. 지정하면 탭 버튼 사이마다 이 위젯을 하나씩 넣는다. */
	UPROPERTY(EditDefaultsOnly, Category = "PADO|UI")
	TSubclassOf<UUserWidget> SeparatorClass;

#if WITH_EDITORONLY_DATA
	/** 이 WBP를 디자이너에서 열었을 때 예시로 그릴 탭 수다. */
	UPROPERTY(EditAnywhere, Category = "PADO|UI|Designer", meta = (ClampMin = "1"))
	int32 DesignerPreviewTabCount = 3;
#endif

private:
	void AddToTabRow(UWidget* Widget);

#if WITH_EDITOR
	void BuildDesignerPreview(const TArray<FText>& Labels, int32 SelectedIndex);
#endif

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HBox_Tabs;

	/** 탭 버튼 앞에 넣은 구분선이다. 탭을 지울 때 함께 지운다. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UUserWidget>> SeparatorsByTab;

#if WITH_EDITORONLY_DATA
	/** 이 탭 목록을 쓰는 화면이 예시를 정했는지다. 정했으면 탭 수 예시로 덮어쓰지 않는다. */
	bool bHasOwnerPreview = false;
#endif
};
