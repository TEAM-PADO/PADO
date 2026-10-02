// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PDSettingRowWidget.generated.h"

class UCommonTextBlock;
class UPDSettingItemViewModel;
class UPDSettingRowFrameWidget;

/**
 * 옵션 창 한 줄의 공용 부모다. 왼쪽에 항목 이름을, 오른쪽에 종류별 조작 위젯을 둔다.
 * 항목 상태가 바뀔 때마다 RefreshFromItem으로 조작 위젯을 다시 맞추고,
 * 편집할 수 없는 항목은 줄 전체를 비활성화한 뒤 이유를 툴팁으로 보여 준다.
 * 줄 모양은 공통 틀(Frame_Row)에 두고, 틀을 쓰지 않는 소제목 줄만 Text_Label을 직접 둔다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSettingRowWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 표시할 옵션 항목을 연결하고 바로 표시를 맞춘다. */
	void SetItem(UPDSettingItemViewModel* InItem);

	/** 키보드·게임패드 초점을 받을 조작 위젯이다. */
	virtual UWidget* GetFocusTarget() const { return nullptr; }

protected:
	/** 항목 상태에 맞춰 표시를 갱신한다. 파생 클래스는 Super를 먼저 호출한다. */
	virtual void RefreshFromItem();

	UPDSettingItemViewModel* GetItem() const { return Item; }

	/** 갱신 중에 조작 위젯이 보내는 변경 이벤트를 사용자 입력으로 처리하지 않기 위한 표시다. */
	bool IsRefreshing() const { return bIsRefreshing; }

private:
	void HandleItemStateChanged(UPDSettingItemViewModel* ChangedItem);

	/** 줄 공통 틀이다. 있으면 항목 이름을 틀에 넣는다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPDSettingRowFrameWidget> Frame_Row;

	/** 공통 틀이 없는 줄(소제목)의 항목 이름이다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Label;

	UPROPERTY(Transient)
	TObjectPtr<UPDSettingItemViewModel> Item;

	FDelegateHandle StateChangedHandle;
	bool bIsRefreshing = false;
};
