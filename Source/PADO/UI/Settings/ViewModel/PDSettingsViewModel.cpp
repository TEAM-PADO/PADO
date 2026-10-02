// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/ViewModel/PDSettingsViewModel.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/Settings/PDKeyBindingSubsystem.h"
#include "PADO/UI/PDUILog.h"
#include "PADO/UI/Settings/PDSettingsText.h"
#include "PADO/UI/Settings/ViewModel/PDSettingChoiceViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingHeaderViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingKeyBindingViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingSliderViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingToggleViewModel.h"
#include "UObject/Package.h"

namespace PDSettingsViewModel
{
	using namespace PDSettingsText::Key;

	struct FWindowModeOption
	{
		EWindowMode::Type WindowMode;
		const TCHAR* LabelKey;
	};

	/** 화면 모드 선택지 순서다. */
	const FWindowModeOption WindowModeOptions[] =
	{
		{ EWindowMode::Fullscreen, DisplayWindowModeFullscreen },
		{ EWindowMode::WindowedFullscreen, DisplayWindowModeBorderless },
		{ EWindowMode::Windowed, DisplayWindowModeWindowed },
	};
	constexpr int32 NumWindowModeOptions = static_cast<int32>(UE_ARRAY_COUNT(WindowModeOptions));

	/** 품질 선택지 문구다. 배열 번호가 엔진 품질 단계와 같다. */
	const TCHAR* const QualityLabelKeys[] =
	{
		GraphicsQualityLow,
		GraphicsQualityMedium,
		GraphicsQualityHigh,
		GraphicsQualityEpic,
	};
	static_assert(static_cast<int32>(UE_ARRAY_COUNT(QualityLabelKeys)) == FPDSettingsValues::MaxSelectableQuality + 1, "Every selectable quality level needs a label.");

	/** 그래픽 품질 선택지에서 품질 단계 다음에 오는 "사용자 지정"의 번호다. */
	constexpr int32 CustomQualityIndex = FPDSettingsValues::MaxSelectableQuality + 1;

	struct FQualityGroupOption
	{
		EPDGraphicsQualityGroup Group;
		const TCHAR* LabelKey;
	};

	/** 화면 탭 세부 품질 순서다. */
	const FQualityGroupOption QualityGroupOptions[] =
	{
		{ EPDGraphicsQualityGroup::ViewDistance, GraphicsViewDistance },
		{ EPDGraphicsQualityGroup::Shadow, GraphicsShadow },
		{ EPDGraphicsQualityGroup::GlobalIllumination, GraphicsGlobalIllumination },
		{ EPDGraphicsQualityGroup::Reflection, GraphicsReflection },
		{ EPDGraphicsQualityGroup::AntiAliasing, GraphicsAntiAliasing },
		{ EPDGraphicsQualityGroup::Texture, GraphicsTexture },
		{ EPDGraphicsQualityGroup::Effects, GraphicsEffects },
		{ EPDGraphicsQualityGroup::PostProcess, GraphicsPostProcess },
		{ EPDGraphicsQualityGroup::Foliage, GraphicsFoliage },
		{ EPDGraphicsQualityGroup::Shading, GraphicsShading },
	};

	struct FVolumeOption
	{
		EPDVolumeCategory Category;
		const TCHAR* LabelKey;
	};

	/** 소리 탭 음량 항목 순서다. */
	const FVolumeOption VolumeOptions[] =
	{
		{ EPDVolumeCategory::Master, AudioMaster },
		{ EPDVolumeCategory::Music, AudioMusic },
		{ EPDVolumeCategory::Effects, AudioEffects },
		{ EPDVolumeCategory::UI, AudioUI },
	};

	struct FCultureOption
	{
		const TCHAR* CultureName;
		/** 언어 이름은 어느 언어에서 보든 그 언어로 적는다. */
		const TCHAR* NativeName;
	};

	const FCultureOption CultureOptions[] =
	{
		{ TEXT("ko"), TEXT("한국어") },
		{ TEXT("en"), TEXT("English") },
	};
	constexpr int32 NumCultureOptions = static_cast<int32>(UE_ARRAY_COUNT(CultureOptions));

	struct FColorVisionOption
	{
		EColorVisionDeficiency Deficiency;
		const TCHAR* LabelKey;
	};

	const FColorVisionOption ColorVisionOptions[] =
	{
		{ EColorVisionDeficiency::NormalVision, CommonOff },
		{ EColorVisionDeficiency::Protanope, AccessibilityColorVisionProtanope },
		{ EColorVisionDeficiency::Deuteranope, AccessibilityColorVisionDeuteranope },
		{ EColorVisionDeficiency::Tritanope, AccessibilityColorVisionTritanope },
	};
	constexpr int32 NumColorVisionOptions = static_cast<int32>(UE_ARRAY_COUNT(ColorVisionOptions));

	const TCHAR* const SubtitleSizeLabelKeys[] =
	{
		GeneralSubtitleSizeSmall,
		GeneralSubtitleSizeMedium,
		GeneralSubtitleSizeLarge,
	};

	/** 성능 정보 선택지 문구다. 배열 번호가 EPDPerformanceInfo 값과 같다. */
	const TCHAR* const PerformanceInfoLabelKeys[] =
	{
		CommonOff,
		GeneralPerformanceInfoFps,
		GeneralPerformanceInfoFpsAndPing,
	};
	constexpr int32 NumPerformanceInfoOptions = static_cast<int32>(UE_ARRAY_COUNT(PerformanceInfoLabelKeys));

	/** 프레임 제한 선택지다. 0은 무제한이다. */
	const TArray<float> FrameRateLimitValues = { 30.0f, 60.0f, 120.0f, 144.0f, 165.0f, 240.0f, 0.0f };

	/** 렌더링 해상도 선택지(%)다. 0은 프로젝트 기본값(자동)이다. */
	const TArray<float> ResolutionScaleValues = { 0.0f, 100.0f, 85.0f, 75.0f, 67.0f, 50.0f };

	constexpr float PercentScale = 100.0f;
	constexpr float PercentStep = 1.0f;
	constexpr float SensitivityStep = 0.1f;
	constexpr float FieldOfViewStep = 1.0f;
	constexpr float UIScaleStep = 5.0f;
	constexpr float ColorVisionSeverityStep = 1.0f;

	bool IsSelectableQuality(const int32 Quality)
	{
		return Quality >= 0 && Quality <= FPDSettingsValues::MaxSelectableQuality;
	}

	FText FormatNamedValue(const TCHAR* FormatKey, const FText& Value)
	{
		FFormatNamedArguments Args;
		Args.Add(PDSettingsText::Arg::Value, Value);
		return FText::Format(PDSettingsText::Get(FormatKey), Args);
	}

	FText FormatWholeNumber(const float Value)
	{
		return FText::AsNumber(FMath::RoundToInt(Value), &FNumberFormattingOptions::DefaultNoGrouping());
	}

	FText FormatOneDecimal(const float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 1;
		Options.MaximumFractionalDigits = 1;
		return FText::AsNumber(Value, &Options);
	}

	FText FormatResolution(const FIntPoint& Resolution)
	{
		FFormatNamedArguments Args;
		Args.Add(PDSettingsText::Arg::Width, FText::AsNumber(Resolution.X, &FNumberFormattingOptions::DefaultNoGrouping()));
		Args.Add(PDSettingsText::Arg::Height, FText::AsNumber(Resolution.Y, &FNumberFormattingOptions::DefaultNoGrouping()));
		return FText::Format(PDSettingsText::Get(DisplayResolutionValue), Args);
	}

	/** 설정에 언어가 비어 있으면 지금 실행 중인 언어를 선택지로 보여 준다. */
	int32 GetCultureIndex(const FString& Culture)
	{
		const FString LanguageName = Culture.IsEmpty()
			? FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName()
			: Culture;

		for (int32 Index = 0; Index < NumCultureOptions; ++Index)
		{
			if (LanguageName.StartsWith(CultureOptions[Index].CultureName))
			{
				return Index;
			}
		}

		return 0;
	}
}

void UPDSettingsViewModel::Initialize(UPDGameUserSettings* InSettings, UPDKeyBindingSubsystem* InKeyBindingSubsystem)
{
	KeyBindingSubsystem = InKeyBindingSubsystem;
	InitializeWithKeyBindings(InSettings, InKeyBindingSubsystem ? InKeyBindingSubsystem->GetKeyBindings() : TArray<FPDKeyBindingEntry>());
}

void UPDSettingsViewModel::InitializeWithKeyBindings(UPDGameUserSettings* InSettings, const TArray<FPDKeyBindingEntry>& InKeyBindings)
{
	Settings = InSettings;
	KeyBindingEntries = InKeyBindings;
	UE_MVVM_SET_PROPERTY_VALUE(bIsAvailable, Settings != nullptr);
	if (!Settings)
	{
		UE_LOG(LogPDUI, Warning, TEXT("Settings screen could not find PDGameUserSettings. Set GameUserSettingsClassName in DefaultEngine.ini."));
	}

	if (Items.IsEmpty())
	{
		BuildItems(KeyBindingEntries);
	}

	ReloadValues();
}

void UPDSettingsViewModel::ReloadValues()
{
	DesktopResolution = Settings ? Settings->GetDesktopResolution() : FIntPoint::ZeroValue;
	AppliedValues = Settings ? FPDSettingsValues::FromSettings(*Settings) : FPDSettingsValues();
	DefaultValues = FPDSettingsValues::MakeDefaults();
	ReadKeyBindings();
	PendingValues = AppliedValues;

	if (Settings)
	{
		Settings->ClearPreview();
	}

	RefreshResolutionOptions();
	RefreshItems();
}

void UPDSettingsViewModel::Apply()
{
	if (!bHasPendingChanges)
	{
		return;
	}

	if (Settings)
	{
		PendingValues.ApplyTo(*Settings);
		Settings->ClearPreview();

		// 화면 모드·해상도를 되돌리는 확인 단계 없이 바로 확정한다.
		Settings->ConfirmVideoMode();
		Settings->ApplySettings(false);
	}

	if (UPDKeyBindingSubsystem* KeyBindings = KeyBindingSubsystem.Get())
	{
		if (!PendingValues.KeyBindings.OrderIndependentCompareEqual(AppliedValues.KeyBindings))
		{
			KeyBindings->ApplyKeyBindings(PendingValues.KeyBindings);
		}
	}

	// 엔진이 검증하며 고친 값까지 다시 읽는다.
	ReloadValues();
}

void UPDSettingsViewModel::Revert()
{
	PendingValues = AppliedValues;
	if (Settings)
	{
		Settings->ClearPreview();
	}

	RefreshResolutionOptions();
	RefreshItems();
}

void UPDSettingsViewModel::ResetTabToDefaults(const EPDSettingsTab Tab)
{
	PendingValues.CopyTab(Tab, DefaultValues);
	if (Tab == EPDSettingsTab::Display)
	{
		RefreshResolutionOptions();
	}

	RefreshItems();
	UpdatePreview();
}

FName UPDSettingsViewModel::FindKeyConflict(const FName MappingName, const FKey& Key) const
{
	for (const TPair<FName, FKey>& Pair : PendingValues.KeyBindings)
	{
		if (Pair.Key != MappingName && Pair.Value == Key)
		{
			return Pair.Key;
		}
	}

	return NAME_None;
}

void UPDSettingsViewModel::AssignKey(const FName MappingName, const FKey& Key)
{
	FKey* CurrentKey = PendingValues.KeyBindings.Find(MappingName);
	if (!CurrentKey || *CurrentKey == Key)
	{
		return;
	}

	const FName ConflictMappingName = FindKeyConflict(MappingName, Key);
	if (FKey* ConflictKey = PendingValues.KeyBindings.Find(ConflictMappingName))
	{
		*ConflictKey = *CurrentKey;
	}

	*CurrentKey = Key;
	RefreshItems();
}

FText UPDSettingsViewModel::GetKeyBindingDisplayName(const FName MappingName) const
{
	const UPDSettingItemViewModel* Item = FindItem(MappingName);
	return Item ? Item->GetDisplayName() : FText::FromName(MappingName);
}

TArray<UPDSettingItemViewModel*> UPDSettingsViewModel::GetItems(const EPDSettingsTab Tab) const
{
	TArray<UPDSettingItemViewModel*> TabItems;
	for (UPDSettingItemViewModel* Item : Items)
	{
		if (Item->GetTab() == Tab)
		{
			TabItems.Add(Item);
		}
	}

	return TabItems;
}

UPDSettingItemViewModel* UPDSettingsViewModel::FindItem(const FName ItemId) const
{
	const TObjectPtr<UPDSettingItemViewModel>* Item = Items.FindByPredicate([ItemId](const UPDSettingItemViewModel* Candidate)
	{
		return Candidate->GetItemId() == ItemId;
	});

	return Item ? Item->Get() : nullptr;
}

void UPDSettingsViewModel::BuildItems(const TArray<FPDKeyBindingEntry>& InKeyBindings)
{
	BuildGeneralItems();
	BuildDisplayItems();
	BuildAudioItems();
	BuildControlsItems();
	BuildKeyBindingItems(InKeyBindings);
	BuildAccessibilityItems();
}

void UPDSettingsViewModel::BuildGeneralItems()
{
	using namespace PDSettingsViewModel;
	constexpr EPDSettingsTab Tab = EPDSettingsTab::General;

	TArray<FText> CultureTexts;
	for (const FCultureOption& Option : CultureOptions)
	{
		CultureTexts.Add(FText::AsCultureInvariant(Option.NativeName));
	}
	AddChoice(Tab, GeneralLanguage, CultureTexts,
		[this]() { return GetCultureIndex(PendingValues.Culture); },
		[this](const int32 Index)
		{
			if (Index >= 0 && Index < NumCultureOptions)
			{
				PendingValues.Culture = CultureOptions[Index].CultureName;
			}
		});

	AddItem<UPDSettingHeaderViewModel>(Tab, GeneralDisplayHeader);

	UPDSettingSliderViewModel* UIScaleItem = AddItem<UPDSettingSliderViewModel>(Tab, GeneralUIScale);
	UIScaleItem->SetRange(UPDGameUserSettings::MinUIScale * PercentScale, UPDGameUserSettings::MaxUIScale * PercentScale, UIScaleStep);
	UIScaleItem->SetAccessors(
		[this]() { return PendingValues.UIScale * PercentScale; },
		[this](const float Value) { PendingValues.UIScale = Value / PercentScale; },
		[](const float Value) { return FormatNamedValue(CommonPercent, FormatWholeNumber(Value)); });

	TArray<FText> PerformanceInfoTexts;
	for (const TCHAR* LabelKey : PerformanceInfoLabelKeys)
	{
		PerformanceInfoTexts.Add(PDSettingsText::Get(LabelKey));
	}
	AddChoice(Tab, GeneralPerformanceInfo, PerformanceInfoTexts,
		[this]() { return static_cast<int32>(PendingValues.PerformanceInfo); },
		[this](const int32 Index)
		{
			if (Index >= 0 && Index < NumPerformanceInfoOptions)
			{
				PendingValues.PerformanceInfo = static_cast<EPDPerformanceInfo>(Index);
			}
		});

	AddItem<UPDSettingHeaderViewModel>(Tab, GeneralSubtitlesHeader);

	AddToggle(Tab, GeneralSubtitles,
		[this]() { return PendingValues.bSubtitlesEnabled; },
		[this](const bool bIsOn) { PendingValues.bSubtitlesEnabled = bIsOn; });

	TArray<FText> SubtitleSizeTexts;
	for (const TCHAR* LabelKey : SubtitleSizeLabelKeys)
	{
		SubtitleSizeTexts.Add(PDSettingsText::Get(LabelKey));
	}
	UPDSettingChoiceViewModel* SubtitleSizeItem = AddChoice(Tab, GeneralSubtitleSize, SubtitleSizeTexts,
		[this]() { return static_cast<int32>(PendingValues.SubtitleSize); },
		[this](const int32 Index) { PendingValues.SubtitleSize = static_cast<EPDSubtitleSize>(Index); });

	UPDSettingToggleViewModel* SubtitleBackgroundItem = AddToggle(Tab, GeneralSubtitleBackground,
		[this]() { return PendingValues.bSubtitleBackground; },
		[this](const bool bIsOn) { PendingValues.bSubtitleBackground = bIsOn; });

	const FText SubtitleLockedReason = PDSettingsText::Get(GeneralSubtitleLocked);
	SubtitleSizeItem->SetEditableCondition([this]() { return bIsAvailable && PendingValues.bSubtitlesEnabled; }, SubtitleLockedReason);
	SubtitleBackgroundItem->SetEditableCondition([this]() { return bIsAvailable && PendingValues.bSubtitlesEnabled; }, SubtitleLockedReason);
}

void UPDSettingsViewModel::BuildDisplayItems()
{
	using namespace PDSettingsViewModel;
	constexpr EPDSettingsTab Tab = EPDSettingsTab::Display;

	TArray<FText> WindowModeTexts;
	for (const FWindowModeOption& Option : WindowModeOptions)
	{
		WindowModeTexts.Add(PDSettingsText::Get(Option.LabelKey));
	}
	AddChoice(Tab, DisplayWindowMode, WindowModeTexts,
		[this]() { return GetWindowModeIndex(); },
		[this](const int32 Index) { SetWindowModeIndex(Index); });

	ResolutionItem = AddChoice(Tab, DisplayResolution, TArray<FText>(),
		[this]() { return ResolutionOptions.IndexOfByKey(PendingValues.Resolution); },
		[this](const int32 Index)
		{
			if (ResolutionOptions.IsValidIndex(Index))
			{
				PendingValues.Resolution = ResolutionOptions[Index];
			}
		});
	ResolutionItem->SetEditableCondition(
		[this]() { return bIsAvailable && PendingValues.WindowMode != EWindowMode::WindowedFullscreen; },
		PDSettingsText::Get(DisplayResolutionLocked));

	AddValueChoice(Tab, DisplayFrameRateLimit, FrameRateLimitValues,
		[](const float Value)
		{
			return Value > 0.0f ? FormatNamedValue(DisplayFrameRateLimitValue, FormatWholeNumber(Value)) : PDSettingsText::Get(CommonUnlimited);
		},
		[this]() { return PendingValues.FrameRateLimit; },
		[this](const float Value) { PendingValues.FrameRateLimit = Value; });

	AddToggle(Tab, DisplayVSync,
		[this]() { return PendingValues.bVSyncEnabled; },
		[this](const bool bIsOn) { PendingValues.bVSyncEnabled = bIsOn; });

	AddPercentSlider(Tab, DisplayBrightness,
		[this]() { return PendingValues.Brightness; },
		[this](const float Value) { PendingValues.Brightness = Value; });

	BuildGraphicsItems(Tab);
}

void UPDSettingsViewModel::BuildGraphicsItems(const EPDSettingsTab Tab)
{
	using namespace PDSettingsViewModel;

	TArray<FText> QualityTexts;
	for (const TCHAR* LabelKey : QualityLabelKeys)
	{
		QualityTexts.Add(PDSettingsText::Get(LabelKey));
	}

	// 품질 단계 뒤에 "사용자 지정"을 둔다. 세부 품질은 이 선택지 없이 단계 문구만 쓴다.
	TArray<FText> PresetTexts = QualityTexts;
	PresetTexts.Add(PDSettingsText::Get(GraphicsQualityCustom));
	QualityItem = AddChoice(Tab, GraphicsQuality, PresetTexts,
		[this]() { return GetQualityIndex(); },
		[this](const int32 Index) { SetQualityIndex(Index); });

	AddValueChoice(Tab, GraphicsResolutionScale, ResolutionScaleValues,
		[](const float Value)
		{
			return Value > 0.0f ? FormatNamedValue(CommonPercent, FormatWholeNumber(Value)) : PDSettingsText::Get(CommonAuto);
		},
		[this]() { return PendingValues.ResolutionScale; },
		[this](const float Value) { PendingValues.ResolutionScale = Value; });

	AddToggle(Tab, GraphicsMotionBlur,
		[this]() { return PendingValues.bMotionBlurEnabled; },
		[this](const bool bIsOn) { PendingValues.bMotionBlurEnabled = bIsOn; });

	UPDSettingSliderViewModel* FieldOfViewItem = AddItem<UPDSettingSliderViewModel>(Tab, GraphicsFieldOfView);
	FieldOfViewItem->SetRange(UPDGameUserSettings::MinFieldOfView, UPDGameUserSettings::MaxFieldOfView, FieldOfViewStep);
	FieldOfViewItem->SetAccessors(
		[this]() { return PendingValues.FieldOfView; },
		[this](const float Value) { PendingValues.FieldOfView = Value; },
		[](const float Value) { return FormatWholeNumber(Value); });

	AddItem<UPDSettingHeaderViewModel>(Tab, GraphicsDetail);

	for (const FQualityGroupOption& Option : QualityGroupOptions)
	{
		const EPDGraphicsQualityGroup Group = Option.Group;
		AddChoice(Tab, Option.LabelKey, QualityTexts,
			[this, Group]() { return PendingValues.GetQualityLevel(Group); },
			[this, Group](const int32 Index)
			{
				// 세부 품질을 직접 고치면 품질 단계는 사용자 지정이 된다.
				PendingValues.SetQualityLevel(Group, Index);
				PendingValues.bCustomQuality = true;
			});
	}
}

void UPDSettingsViewModel::BuildAudioItems()
{
	for (const PDSettingsViewModel::FVolumeOption& Option : PDSettingsViewModel::VolumeOptions)
	{
		const EPDVolumeCategory Category = Option.Category;
		AddPercentSlider(EPDSettingsTab::Audio, Option.LabelKey,
			[this, Category]() { return PendingValues.GetVolume(Category); },
			[this, Category](const float Value) { PendingValues.SetVolume(Category, Value); });
	}
}

void UPDSettingsViewModel::BuildControlsItems()
{
	using namespace PDSettingsViewModel;
	constexpr EPDSettingsTab Tab = EPDSettingsTab::Controls;

	UPDSettingSliderViewModel* MouseSensitivityItem = AddItem<UPDSettingSliderViewModel>(Tab, ControlsMouseSensitivity);
	MouseSensitivityItem->SetRange(UPDGameUserSettings::MinMouseSensitivity, UPDGameUserSettings::MaxMouseSensitivity, SensitivityStep);
	MouseSensitivityItem->SetAccessors(
		[this]() { return PendingValues.MouseSensitivity; },
		[this](const float Value) { PendingValues.MouseSensitivity = Value; },
		&FormatOneDecimal);

	UPDSettingSliderViewModel* AimSensitivityItem = AddItem<UPDSettingSliderViewModel>(Tab, ControlsAimSensitivity);
	AimSensitivityItem->SetRange(UPDGameUserSettings::MinAimSensitivity, UPDGameUserSettings::MaxAimSensitivity, SensitivityStep);
	AimSensitivityItem->SetAccessors(
		[this]() { return PendingValues.AimSensitivity; },
		[this](const float Value) { PendingValues.AimSensitivity = Value; },
		&FormatOneDecimal);

	AddToggle(Tab, ControlsInvertMouseY,
		[this]() { return PendingValues.bInvertMouseY; },
		[this](const bool bIsOn) { PendingValues.bInvertMouseY = bIsOn; });
}

void UPDSettingsViewModel::BuildKeyBindingItems(const TArray<FPDKeyBindingEntry>& InKeyBindings)
{
	constexpr EPDSettingsTab Tab = EPDSettingsTab::KeyBindings;
	if (InKeyBindings.IsEmpty())
	{
		AddItem<UPDSettingHeaderViewModel>(Tab, PDSettingsText::Key::KeyBindingsEmpty);
		return;
	}

	for (const FPDKeyBindingEntry& Entry : InKeyBindings)
	{
		const FName MappingName = Entry.MappingName;
		const FText AssetName = Entry.DisplayName.IsEmpty() ? FText::FromName(MappingName) : Entry.DisplayName;
		const FText DisplayName = PDSettingsText::GetOrFallback(FString(PDSettingsText::KeyBindingLabelPrefix) + MappingName.ToString(), AssetName);

		UPDSettingKeyBindingViewModel* KeyItem = AddItem<UPDSettingKeyBindingViewModel>(Tab, MappingName, DisplayName);
		KeyItem->SetMapping(MappingName, [this, MappingName]()
		{
			const FKey* Key = PendingValues.KeyBindings.Find(MappingName);
			return Key ? *Key : EKeys::Invalid;
		});

		// 키 설정은 GameUserSettings가 아니라 Enhanced Input 사용자 설정을 쓰므로 항상 편집할 수 있다.
		KeyItem->SetEditableCondition(nullptr, FText::GetEmpty());
		KeyItem->OnKeyCaptureRequested.AddUObject(this, &ThisClass::HandleKeyCaptureRequested);
	}
}

void UPDSettingsViewModel::BuildAccessibilityItems()
{
	using namespace PDSettingsViewModel;
	constexpr EPDSettingsTab Tab = EPDSettingsTab::Accessibility;

	TArray<FText> ColorVisionTexts;
	for (const FColorVisionOption& Option : ColorVisionOptions)
	{
		ColorVisionTexts.Add(PDSettingsText::Get(Option.LabelKey));
	}
	AddChoice(Tab, AccessibilityColorVision, ColorVisionTexts,
		[this]()
		{
			for (int32 Index = 0; Index < NumColorVisionOptions; ++Index)
			{
				if (ColorVisionOptions[Index].Deficiency == PendingValues.ColorVisionDeficiency)
				{
					return Index;
				}
			}
			return 0;
		},
		[this](const int32 Index)
		{
			if (Index >= 0 && Index < NumColorVisionOptions)
			{
				PendingValues.ColorVisionDeficiency = ColorVisionOptions[Index].Deficiency;
			}
		});

	UPDSettingSliderViewModel* SeverityItem = AddItem<UPDSettingSliderViewModel>(Tab, AccessibilityColorVisionSeverity);
	SeverityItem->SetRange(0.0f, static_cast<float>(UPDGameUserSettings::MaxColorVisionSeverity), ColorVisionSeverityStep);
	SeverityItem->SetAccessors(
		[this]() { return static_cast<float>(PendingValues.ColorVisionSeverity); },
		[this](const float Value) { PendingValues.ColorVisionSeverity = FMath::RoundToInt(Value); },
		[](const float Value) { return FormatWholeNumber(Value); });
	SeverityItem->SetEditableCondition(
		[this]() { return bIsAvailable && PendingValues.ColorVisionDeficiency != EColorVisionDeficiency::NormalVision; },
		PDSettingsText::Get(AccessibilityColorVisionLocked));

	AddPercentSlider(Tab, AccessibilityCameraShake,
		[this]() { return PendingValues.CameraShakeScale; },
		[this](const float Value) { PendingValues.CameraShakeScale = Value; });
}

template <typename ItemT>
ItemT* UPDSettingsViewModel::AddItem(const EPDSettingsTab Tab, const FName ItemId, const FText& DisplayName)
{
	ItemT* Item = NewObject<ItemT>(this);
	Item->InitializeItem(ItemId, Tab, DisplayName);
	Item->SetEditableCondition([this]() { return bIsAvailable; }, PDSettingsText::Get(PDSettingsText::Key::CommonUnavailable));
	Item->OnEdited.AddUObject(this, &ThisClass::HandleItemEdited);
	Items.Add(Item);
	return Item;
}

template <typename ItemT>
ItemT* UPDSettingsViewModel::AddItem(const EPDSettingsTab Tab, const TCHAR* LabelKey)
{
	return AddItem<ItemT>(Tab, FName(LabelKey), PDSettingsText::Get(LabelKey));
}

UPDSettingChoiceViewModel* UPDSettingsViewModel::AddChoice(const EPDSettingsTab Tab, const TCHAR* LabelKey, const TArray<FText>& Options,
	TFunction<int32()> InGetter, TFunction<void(int32)> InSetter)
{
	UPDSettingChoiceViewModel* Item = AddItem<UPDSettingChoiceViewModel>(Tab, LabelKey);
	Item->SetOptions(Options);
	Item->SetAccessors(MoveTemp(InGetter), MoveTemp(InSetter));
	return Item;
}

UPDSettingChoiceViewModel* UPDSettingsViewModel::AddValueChoice(const EPDSettingsTab Tab, const TCHAR* LabelKey, const TArray<float>& Values,
	TFunction<FText(float)> InFormatter, TFunction<float()> InGetter, TFunction<void(float)> InSetter)
{
	UPDSettingChoiceViewModel* Item = AddItem<UPDSettingChoiceViewModel>(Tab, LabelKey);
	Item->SetValueAccessors(Values, MoveTemp(InFormatter), MoveTemp(InGetter), MoveTemp(InSetter));
	return Item;
}

UPDSettingToggleViewModel* UPDSettingsViewModel::AddToggle(const EPDSettingsTab Tab, const TCHAR* LabelKey, TFunction<bool()> InGetter, TFunction<void(bool)> InSetter)
{
	UPDSettingToggleViewModel* Item = AddItem<UPDSettingToggleViewModel>(Tab, LabelKey);
	Item->SetValueTexts(PDSettingsText::Get(PDSettingsText::Key::CommonOn), PDSettingsText::Get(PDSettingsText::Key::CommonOff));
	Item->SetAccessors(MoveTemp(InGetter), MoveTemp(InSetter));
	return Item;
}

UPDSettingSliderViewModel* UPDSettingsViewModel::AddPercentSlider(const EPDSettingsTab Tab, const TCHAR* LabelKey, TFunction<float()> InGetter, TFunction<void(float)> InSetter)
{
	using namespace PDSettingsViewModel;

	UPDSettingSliderViewModel* Item = AddItem<UPDSettingSliderViewModel>(Tab, LabelKey);
	Item->SetRange(0.0f, PercentScale, PercentStep);
	Item->SetAccessors(
		[Getter = MoveTemp(InGetter)]() { return Getter() * PercentScale; },
		[Setter = MoveTemp(InSetter)](const float Value) { Setter(Value / PercentScale); },
		&FormatWholeNumber);
	return Item;
}

void UPDSettingsViewModel::RefreshResolutionOptions()
{
	TArray<FIntPoint> NewOptions;
	if (PendingValues.WindowMode == EWindowMode::WindowedFullscreen)
	{
		// 테두리 없는 창은 항상 바탕 화면 해상도로 그린다.
		NewOptions.Add(DesktopResolution);
	}
	else
	{
		TArray<FIntPoint> PlatformResolutions;
		if (PendingValues.WindowMode == EWindowMode::Fullscreen)
		{
			UKismetSystemLibrary::GetSupportedFullscreenResolutions(PlatformResolutions);
		}
		else
		{
			UKismetSystemLibrary::GetConvenientWindowedResolutions(PlatformResolutions);
		}

		for (const FIntPoint& Resolution : PlatformResolutions)
		{
			if (Resolution.X > 0 && Resolution.Y > 0)
			{
				NewOptions.AddUnique(Resolution);
			}
		}
	}

	if (PendingValues.Resolution.X > 0 && PendingValues.Resolution.Y > 0)
	{
		NewOptions.AddUnique(PendingValues.Resolution);
	}

	// 큰 해상도가 위에 오게 한다.
	NewOptions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X != B.X ? A.X > B.X : A.Y > B.Y;
	});

	if (NewOptions == ResolutionOptions && ResolutionItem->GetOptions().Num() == NewOptions.Num())
	{
		return;
	}

	ResolutionOptions = MoveTemp(NewOptions);
	TArray<FText> ResolutionTexts;
	for (const FIntPoint& Resolution : ResolutionOptions)
	{
		ResolutionTexts.Add(PDSettingsViewModel::FormatResolution(Resolution));
	}

	ResolutionItem->SetOptions(ResolutionTexts);
}

void UPDSettingsViewModel::RefreshItems()
{
	for (UPDSettingItemViewModel* Item : Items)
	{
		Item->Refresh();
	}

	const bool bCanApplyValues = bIsAvailable || KeyBindingSubsystem.IsValid();
	UE_MVVM_SET_PROPERTY_VALUE(bHasPendingChanges, bCanApplyValues && PendingValues != AppliedValues);
}

void UPDSettingsViewModel::UpdatePreview() const
{
	if (Settings)
	{
		Settings->SetPreview(PendingValues.Brightness, PendingValues.Volumes);
	}
}

void UPDSettingsViewModel::ReadKeyBindings()
{
	if (const UPDKeyBindingSubsystem* KeyBindings = KeyBindingSubsystem.Get())
	{
		KeyBindingEntries = KeyBindings->GetKeyBindings();
	}

	AppliedValues.KeyBindings.Reset();
	DefaultValues.KeyBindings.Reset();
	for (const FPDKeyBindingEntry& Entry : KeyBindingEntries)
	{
		AppliedValues.KeyBindings.Add(Entry.MappingName, Entry.CurrentKey);
		DefaultValues.KeyBindings.Add(Entry.MappingName, Entry.DefaultKey);
	}
}

void UPDSettingsViewModel::HandleItemEdited(UPDSettingItemViewModel* Item)
{
	// 한 항목이 다른 항목의 편집 가능 여부나 선택지를 바꿀 수 있어 모든 줄을 다시 맞춘다.
	RefreshItems();
	UpdatePreview();
}

void UPDSettingsViewModel::HandleKeyCaptureRequested(UPDSettingItemViewModel* Item)
{
	if (UPDSettingKeyBindingViewModel* KeyItem = Cast<UPDSettingKeyBindingViewModel>(Item))
	{
		OnKeyCaptureRequested.Broadcast(KeyItem);
	}
}

int32 UPDSettingsViewModel::GetWindowModeIndex() const
{
	for (int32 Index = 0; Index < PDSettingsViewModel::NumWindowModeOptions; ++Index)
	{
		if (PDSettingsViewModel::WindowModeOptions[Index].WindowMode == PendingValues.WindowMode)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UPDSettingsViewModel::SetWindowModeIndex(const int32 Index)
{
	if (Index < 0 || Index >= PDSettingsViewModel::NumWindowModeOptions)
	{
		return;
	}

	PendingValues.WindowMode = PDSettingsViewModel::WindowModeOptions[Index].WindowMode;
	if (PendingValues.WindowMode == EWindowMode::WindowedFullscreen)
	{
		PendingValues.Resolution = DesktopResolution;
	}

	RefreshResolutionOptions();
}

int32 UPDSettingsViewModel::GetQualityIndex() const
{
	// 사용자 지정을 골랐거나 세부 품질이 서로 다르면 사용자 지정으로 보여 준다.
	const int32 OverallQuality = PendingValues.GetOverallQuality();
	if (PendingValues.bCustomQuality || !PDSettingsViewModel::IsSelectableQuality(OverallQuality))
	{
		return PDSettingsViewModel::CustomQualityIndex;
	}

	return OverallQuality;
}

void UPDSettingsViewModel::SetQualityIndex(const int32 Index)
{
	if (PDSettingsViewModel::IsSelectableQuality(Index))
	{
		PendingValues.SetOverallQuality(Index);
		PendingValues.bCustomQuality = false;
	}
	else if (Index == PDSettingsViewModel::CustomQualityIndex)
	{
		// 사용자 지정은 지금 세부 품질을 그대로 두고 세부 품질을 직접 고르는 상태로 바꾼다.
		PendingValues.bCustomQuality = true;
	}
}
