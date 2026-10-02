// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PADO/Settings/Struct/PDKeyBindingEntry.h"
#include "PADO/UI/Settings/Enum/PDSettingsTab.h"
#include "PADO/UI/Settings/Struct/PDSettingsValues.h"
#include "PDSettingsViewModel.generated.h"

class UPDGameUserSettings;
class UPDKeyBindingSubsystem;
class UPDSettingChoiceViewModel;
class UPDSettingItemViewModel;
class UPDSettingKeyBindingViewModel;
class UPDSettingSliderViewModel;
class UPDSettingToggleViewModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FPDSettingsKeyCaptureSignature, UPDSettingKeyBindingViewModel*);

/**
 * 옵션 창 전체 상태다. 탭별 옵션 항목을 만들고, 편집 중인 값과 적용된 값을 따로 들고 있다.
 * 값은 적용을 누를 때만 저장하고, 밝기와 음량만 편집하는 동안 미리 반영한다(취소하면 되돌린다).
 * 새 옵션은 FPDSettingsValues에 값을 더하고 Build*Items에 항목 하나를 추가해 만든다.
 */
UCLASS(BlueprintType)
class PADO_API UPDSettingsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 옵션 항목을 만들고 현재 값을 읽는다. 설정 객체가 없으면 키 설정 외 항목을 편집할 수 없게 둔다. */
	void Initialize(UPDGameUserSettings* InSettings, UPDKeyBindingSubsystem* InKeyBindingSubsystem);

	/** 키 설정 목록을 직접 넘겨 초기화한다. 키 설정 서브시스템이 없는 자동화 테스트에서 쓴다. */
	void InitializeWithKeyBindings(UPDGameUserSettings* InSettings, const TArray<FPDKeyBindingEntry>& InKeyBindings);

	/** 설정에서 값을 다시 읽고 편집 중인 값과 미리보기를 버린다. 창을 열 때마다 호출한다. */
	void ReloadValues();

	/** 편집 중인 값을 엔진에 적용하고 저장한다. 화면 모드·해상도도 확인 없이 바로 확정한다. */
	void Apply();

	/** 편집 중인 값을 마지막으로 적용한 값으로 되돌리고 미리보기를 끝낸다. */
	void Revert();

	/** 지정한 탭의 편집 중인 값만 기본값으로 바꾼다. 적용은 하지 않는다. */
	void ResetTabToDefaults(EPDSettingsTab Tab);

	/** 같은 키를 이미 쓰고 있는 다른 동작의 매핑 이름이다. 없으면 NAME_None이다. */
	FName FindKeyConflict(FName MappingName, const FKey& Key) const;

	/** 키를 바꾼다. 다른 동작이 같은 키를 쓰고 있으면 두 동작의 키를 맞바꾼다. */
	void AssignKey(FName MappingName, const FKey& Key);

	/** 키 설정 줄에 보이는 동작 이름이다. */
	FText GetKeyBindingDisplayName(FName MappingName) const;

	/** 지정한 탭에 속한 항목을 화면 순서대로 반환한다. */
	TArray<UPDSettingItemViewModel*> GetItems(EPDSettingsTab Tab) const;

	/** 항목 식별자(이름 문구의 String Table 키, 키 설정은 매핑 이름)로 항목을 찾는다. */
	UPDSettingItemViewModel* FindItem(FName ItemId) const;

	bool IsAvailable() const { return bIsAvailable; }
	bool HasPendingChanges() const { return bHasPendingChanges; }
	const FPDSettingsValues& GetPendingValues() const { return PendingValues; }

	/** 키 설정 줄의 키 버튼을 눌렀을 때 보낸다. 옵션 화면이 키 입력 대기 창을 띄운다. */
	FPDSettingsKeyCaptureSignature OnKeyCaptureRequested;

private:
	void BuildItems(const TArray<FPDKeyBindingEntry>& InKeyBindings);
	void BuildGeneralItems();
	void BuildDisplayItems();
	void BuildGraphicsItems(EPDSettingsTab Tab);
	void BuildAudioItems();
	void BuildControlsItems();
	void BuildKeyBindingItems(const TArray<FPDKeyBindingEntry>& InKeyBindings);
	void BuildAccessibilityItems();

	template <typename ItemT>
	ItemT* AddItem(EPDSettingsTab Tab, FName ItemId, const FText& DisplayName);

	template <typename ItemT>
	ItemT* AddItem(EPDSettingsTab Tab, const TCHAR* LabelKey);

	UPDSettingChoiceViewModel* AddChoice(EPDSettingsTab Tab, const TCHAR* LabelKey, const TArray<FText>& Options,
		TFunction<int32()> InGetter, TFunction<void(int32)> InSetter);

	/** 숫자 목록 중 하나를 고르는 항목을 추가한다. 지금 값이 목록에 없으면 그 값을 선택지 끝에 덧붙여 보여 준다. */
	UPDSettingChoiceViewModel* AddValueChoice(EPDSettingsTab Tab, const TCHAR* LabelKey, const TArray<float>& Values,
		TFunction<FText(float)> InFormatter, TFunction<float()> InGetter, TFunction<void(float)> InSetter);

	UPDSettingToggleViewModel* AddToggle(EPDSettingsTab Tab, const TCHAR* LabelKey, TFunction<bool()> InGetter, TFunction<void(bool)> InSetter);

	/** 0~1 값을 0~100으로 보여 주는 슬라이더 항목을 추가한다. */
	UPDSettingSliderViewModel* AddPercentSlider(EPDSettingsTab Tab, const TCHAR* LabelKey, TFunction<float()> InGetter, TFunction<void(float)> InSetter);

	void RefreshResolutionOptions();
	void RefreshItems();
	void UpdatePreview() const;
	void ReadKeyBindings();
	void HandleItemEdited(UPDSettingItemViewModel* Item);
	void HandleKeyCaptureRequested(UPDSettingItemViewModel* Item);

	int32 GetWindowModeIndex() const;
	void SetWindowModeIndex(int32 Index);
	int32 GetQualityIndex() const;
	void SetQualityIndex(int32 Index);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	bool bIsAvailable = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Settings", meta = (AllowPrivateAccess = "true"))
	bool bHasPendingChanges = false;

	UPROPERTY(Transient)
	TObjectPtr<UPDGameUserSettings> Settings;

	TWeakObjectPtr<UPDKeyBindingSubsystem> KeyBindingSubsystem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDSettingItemViewModel>> Items;

	UPROPERTY(Transient)
	TObjectPtr<UPDSettingChoiceViewModel> ResolutionItem;

	UPROPERTY(Transient)
	TObjectPtr<UPDSettingChoiceViewModel> QualityItem;

	FPDSettingsValues AppliedValues;
	FPDSettingsValues PendingValues;
	FPDSettingsValues DefaultValues;
	FIntPoint DesktopResolution = FIntPoint::ZeroValue;
	TArray<FIntPoint> ResolutionOptions;
	TArray<FPDKeyBindingEntry> KeyBindingEntries;
};
