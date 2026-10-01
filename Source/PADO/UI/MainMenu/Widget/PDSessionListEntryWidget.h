// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CommonUserWidget.h"
#include "PDSessionListEntryWidget.generated.h"

class UWidget;

/**
 * 게임 참가 목록의 방 한 줄이다.
 * 목록이 넘겨준 UPDSessionEntryViewModel을 MVVM View에 연결하고, 표시는 WBP 바인딩이 맡는다.
 * 목록 행은 선택 강조를 그리지 않으므로 선택 표시는 이 위젯이 Image_Selected로 직접 한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDSessionListEntryWidget : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

private:
	/** 선택된 방에만 보이는 강조 위젯이다. 없으면 강조 없이 동작한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Image_Selected;
};
