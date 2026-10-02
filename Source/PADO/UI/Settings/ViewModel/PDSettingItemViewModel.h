// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PADO/UI/Settings/Enum/PDSettingsTab.h"
#include "PDSettingItemViewModel.generated.h"

class UPDSettingItemViewModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FPDSettingItemSignature, UPDSettingItemViewModel*);

/**
 * 옵션 창 한 줄의 공용 상태다. 값은 소유자가 넘긴 함수로 읽고 쓴다.
 * 사용자가 값을 바꾸면 OnEdited로 소유자에게 알리고, 소유자는 Refresh로 모든 줄의 표시를 다시 맞춘다.
 * 표시가 바뀔 때마다 OnStateChanged를 보내 줄 위젯이 조작 위젯을 갱신하게 한다.
 */
UCLASS(Abstract, BlueprintType)
class PADO_API UPDSettingItemViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 항목 식별자·탭·표시 이름을 지정한다. 식별자는 이름 문구의 String Table 키를 쓴다. */
	void InitializeItem(FName InItemId, EPDSettingsTab InTab, const FText& InDisplayName);

	/** 편집 가능 조건과 편집할 수 없을 때 보여 줄 이유를 지정한다. 지정하지 않으면 항상 편집할 수 있다. */
	void SetEditableCondition(TFunction<bool()> InEditableGetter, const FText& InDisabledReason);

	/** 소유자 값에서 편집 가능 여부와 값을 다시 읽고 OnStateChanged를 보낸다. */
	void Refresh();

	FName GetItemId() const { return ItemId; }
	EPDSettingsTab GetTab() const { return Tab; }
	const FText& GetDisplayName() const { return DisplayName; }
	bool IsEditable() const { return bIsEditable; }
	const FText& GetDisabledReason() const { return DisabledReason; }

	/** 사용자가 값을 바꿨을 때 보낸다. */
	FPDSettingItemSignature OnEdited;

	/** 표시 상태를 다시 읽은 뒤 보낸다. */
	FPDSettingItemSignature OnStateChanged;

protected:
	/** 종류별 값을 소유자 값에서 다시 읽는다. */
	virtual void RefreshValue() {}

	/** 사용자 입력으로 값을 바꾼 뒤 호출한다. */
	void NotifyEdited();

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	bool bIsEditable = true;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	FText DisabledReason;

	FName ItemId;
	EPDSettingsTab Tab = EPDSettingsTab::Display;
	TFunction<bool()> EditableGetter;
	FText ConditionDisabledReason;
};
