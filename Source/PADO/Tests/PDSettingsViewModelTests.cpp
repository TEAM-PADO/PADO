#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/Settings/Struct/PDKeyBindingEntry.h"
#include "PADO/UI/Settings/PDSettingsDesignerPreview.h"
#include "PADO/UI/Settings/PDSettingsText.h"
#include "PADO/UI/Settings/Struct/PDSettingsValues.h"
#include "PADO/UI/Settings/ViewModel/PDSettingChoiceViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingHeaderViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingKeyBindingViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingSliderViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingToggleViewModel.h"
#include "PADO/UI/Settings/ViewModel/PDSettingsViewModel.h"
#include "UObject/Package.h"

namespace PDSettingsViewModelTests
{
	/** 엔진이 쓰는 설정 객체를 건드리지 않도록 테스트마다 임시 설정 객체를 만든다. */
	UPDGameUserSettings* MakeSettings()
	{
		UPDGameUserSettings* Settings = NewObject<UPDGameUserSettings>(GetTransientPackage());
		Settings->SetToDefaults();
		return Settings;
	}

	FPDKeyBindingEntry MakeKeyBinding(const TCHAR* MappingName, const FKey& Key)
	{
		FPDKeyBindingEntry Entry;
		Entry.MappingName = MappingName;
		Entry.DisplayName = FText::FromString(MappingName);
		Entry.CurrentKey = Key;
		Entry.DefaultKey = Key;
		return Entry;
	}

	UPDSettingsViewModel* MakeViewModel(UPDGameUserSettings* Settings, const TArray<FPDKeyBindingEntry>& KeyBindings = TArray<FPDKeyBindingEntry>())
	{
		UPDSettingsViewModel* ViewModel = NewObject<UPDSettingsViewModel>(GetTransientPackage());
		ViewModel->InitializeWithKeyBindings(Settings, KeyBindings);
		return ViewModel;
	}

	template <typename ItemT>
	ItemT* FindItem(const UPDSettingsViewModel* ViewModel, const TCHAR* ItemKey)
	{
		return Cast<ItemT>(ViewModel->FindItem(FName(ItemKey)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDGameUserSettingsClampTest,
	"PADO.UI.Settings.GameUserSettings.Clamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDGameUserSettingsClampTest::RunTest(const FString& Parameters)
{
	UPDGameUserSettings* Settings = PDSettingsViewModelTests::MakeSettings();

	TestEqual(TEXT("An unsaved volume defaults to full."), Settings->GetVolume(EPDVolumeCategory::Music), UPDGameUserSettings::DefaultVolume);

	Settings->SetBrightness(2.0f);
	TestEqual(TEXT("Brightness is clamped to 1."), Settings->GetBrightness(), 1.0f);

	Settings->SetVolume(EPDVolumeCategory::Effects, -1.0f);
	TestEqual(TEXT("Volume is clamped to 0."), Settings->GetVolume(EPDVolumeCategory::Effects), 0.0f);

	Settings->SetMouseSensitivity(100.0f);
	TestEqual(TEXT("Mouse sensitivity is clamped to the maximum."), Settings->GetMouseSensitivity(), UPDGameUserSettings::MaxMouseSensitivity);

	Settings->SetFieldOfView(10.0f);
	TestEqual(TEXT("Field of view is clamped to the minimum."), Settings->GetFieldOfView(), UPDGameUserSettings::MinFieldOfView);

	Settings->SetUIScale(5.0f);
	TestEqual(TEXT("UI scale is clamped to the maximum."), Settings->GetUIScale(), UPDGameUserSettings::MaxUIScale);

	TestTrue(TEXT("Default brightness keeps the engine display gamma."),
		FMath::IsNearlyEqual(UPDGameUserSettings::BrightnessToDisplayGamma(UPDGameUserSettings::DefaultBrightness), 2.2f));

	Settings->SetPreview(0.9f, { { EPDVolumeCategory::Music, 0.3f } });
	TestEqual(TEXT("A previewed volume is used for playback."), Settings->GetEffectiveVolume(EPDVolumeCategory::Music), 0.3f);
	TestEqual(TEXT("A preview does not change the saved volume."), Settings->GetVolume(EPDVolumeCategory::Music), UPDGameUserSettings::DefaultVolume);
	Settings->ClearPreview();
	TestEqual(TEXT("Clearing the preview restores the saved volume."), Settings->GetEffectiveVolume(EPDVolumeCategory::Music), UPDGameUserSettings::DefaultVolume);

	Settings->SetToDefaults();
	TestEqual(TEXT("Defaults reset brightness."), Settings->GetBrightness(), UPDGameUserSettings::DefaultBrightness);
	TestEqual(TEXT("Defaults reset volume."), Settings->GetVolume(EPDVolumeCategory::Effects), UPDGameUserSettings::DefaultVolume);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsValuesRoundTripTest,
	"PADO.UI.Settings.Values.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsValuesRoundTripTest::RunTest(const FString& Parameters)
{
	UPDGameUserSettings* Settings = PDSettingsViewModelTests::MakeSettings();

	FPDSettingsValues Values = FPDSettingsValues::FromSettings(*Settings);
	Values.WindowMode = EWindowMode::Windowed;
	Values.Resolution = FIntPoint(1280, 720);
	Values.FrameRateLimit = 144.0f;
	Values.bVSyncEnabled = true;
	Values.Brightness = 0.7f;
	Values.SetOverallQuality(1);
	Values.SetQualityLevel(EPDGraphicsQualityGroup::Shadow, 3);
	Values.bCustomQuality = true;
	Values.ResolutionScale = 75.0f;
	Values.bMotionBlurEnabled = false;
	Values.FieldOfView = 100.0f;
	Values.SetVolume(EPDVolumeCategory::Music, 0.25f);
	Values.MouseSensitivity = 1.5f;
	Values.AimSensitivity = 0.5f;
	Values.bInvertMouseY = true;
	Values.UIScale = 1.2f;
	Values.PerformanceInfo = EPDPerformanceInfo::FpsAndPing;
	Values.bSubtitlesEnabled = false;
	Values.SubtitleSize = EPDSubtitleSize::Large;
	Values.bSubtitleBackground = false;
	Values.Culture = TEXT("en");
	Values.ColorVisionDeficiency = EColorVisionDeficiency::Protanope;
	Values.ColorVisionSeverity = 8;
	Values.CameraShakeScale = 0.4f;
	Values.ApplyTo(*Settings);

	TestTrue(TEXT("Values written to the settings object read back unchanged."), FPDSettingsValues::FromSettings(*Settings) == Values);
	TestEqual(TEXT("Mixed detail levels are a custom quality."), Values.GetOverallQuality(), static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelPendingChangesTest,
	"PADO.UI.Settings.ViewModel.PendingChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelPendingChangesTest::RunTest(const FString& Parameters)
{
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(PDSettingsViewModelTests::MakeSettings());
	UPDSettingSliderViewModel* Brightness = PDSettingsViewModelTests::FindItem<UPDSettingSliderViewModel>(ViewModel, PDSettingsText::Key::DisplayBrightness);
	if (!TestNotNull(TEXT("The display tab has a brightness slider."), Brightness))
	{
		return false;
	}

	TestFalse(TEXT("A freshly opened screen has nothing to apply."), ViewModel->HasPendingChanges());
	TestEqual(TEXT("Brightness is shown on a 0-100 scale."), Brightness->GetValue(), 50.0f);

	Brightness->SetValueFromUser(80.4f);
	TestTrue(TEXT("Moving a slider creates a pending change."), ViewModel->HasPendingChanges());
	TestEqual(TEXT("The slider snaps to whole steps."), Brightness->GetValue(), 80.0f);
	TestTrue(TEXT("The pending value is stored as 0-1."), FMath::IsNearlyEqual(ViewModel->GetPendingValues().Brightness, 0.8f));

	Brightness->SetValueFromUser(50.0f);
	TestFalse(TEXT("Returning to the applied value clears the pending change."), ViewModel->HasPendingChanges());

	Brightness->SetValueFromUser(10.0f);
	ViewModel->Revert();
	TestFalse(TEXT("Revert discards pending changes."), ViewModel->HasPendingChanges());
	TestEqual(TEXT("Revert restores the slider."), Brightness->GetValue(), 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelBorderlessTest,
	"PADO.UI.Settings.ViewModel.BorderlessLocksResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelBorderlessTest::RunTest(const FString& Parameters)
{
	UPDGameUserSettings* Settings = PDSettingsViewModelTests::MakeSettings();
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(Settings);
	UPDSettingChoiceViewModel* WindowMode = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::DisplayWindowMode);
	UPDSettingChoiceViewModel* Resolution = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::DisplayResolution);
	if (!TestNotNull(TEXT("The display tab has a window mode choice."), WindowMode) || !TestNotNull(TEXT("The display tab has a resolution choice."), Resolution))
	{
		return false;
	}

	// 선택지 순서: 전체 화면, 테두리 없는 창, 창 모드
	WindowMode->SelectOption(2);
	TestEqual(TEXT("Windowed mode is pending."), ViewModel->GetPendingValues().WindowMode, EWindowMode::Windowed);
	TestTrue(TEXT("Windowed mode lets the player choose a resolution."), Resolution->IsEditable());
	TestTrue(TEXT("The pending resolution is one of the choices."), Resolution->GetSelectedIndex() != INDEX_NONE);

	WindowMode->SelectOption(1);
	TestFalse(TEXT("Borderless mode locks the resolution."), Resolution->IsEditable());
	TestFalse(TEXT("A locked resolution explains why."), Resolution->GetDisabledReason().IsEmpty());
	TestEqual(TEXT("Borderless mode uses the desktop resolution."), ViewModel->GetPendingValues().Resolution, Settings->GetDesktopResolution());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelQualityPresetTest,
	"PADO.UI.Settings.ViewModel.QualityPreset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelQualityPresetTest::RunTest(const FString& Parameters)
{
	constexpr int32 CustomIndex = FPDSettingsValues::MaxSelectableQuality + 1;

	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(PDSettingsViewModelTests::MakeSettings());
	UPDSettingChoiceViewModel* Quality = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::GraphicsQuality);
	UPDSettingChoiceViewModel* Shadow = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::GraphicsShadow);
	if (!TestNotNull(TEXT("The display tab has a quality choice."), Quality) || !TestNotNull(TEXT("The display tab has a shadow choice."), Shadow))
	{
		return false;
	}

	TestEqual(TEXT("Quality offers every level and a custom choice."), Quality->GetOptions().Num(), CustomIndex + 1);
	TestEqual(TEXT("New settings start at medium quality."), Quality->GetSelectedIndex(), UPDGameUserSettings::DefaultQualityLevel);
	TestEqual(TEXT("Detail rows have no custom choice."), Shadow->GetOptions().Num(), FPDSettingsValues::MaxSelectableQuality + 1);

	Quality->SelectOption(2);
	TestEqual(TEXT("A preset changes every detail level."), ViewModel->GetPendingValues().GetQualityLevel(EPDGraphicsQualityGroup::Texture), 2);
	TestEqual(TEXT("The detail row follows the preset."), Shadow->GetSelectedIndex(), 2);

	Shadow->SelectOption(3);
	TestEqual(TEXT("Editing a detail selects the custom choice."), Quality->GetSelectedIndex(), CustomIndex);
	TestEqual(TEXT("Editing a detail keeps the other details."), ViewModel->GetPendingValues().GetQualityLevel(EPDGraphicsQualityGroup::Texture), 2);

	Quality->SelectOption(1);
	TestEqual(TEXT("Choosing a preset again aligns the details."), Shadow->GetSelectedIndex(), 1);
	TestFalse(TEXT("Choosing a preset leaves custom quality."), ViewModel->GetPendingValues().bCustomQuality);

	Quality->SelectOption(CustomIndex);
	TestEqual(TEXT("Custom can be chosen while the details match."), Quality->GetSelectedIndex(), CustomIndex);
	TestEqual(TEXT("Choosing custom keeps the details."), Shadow->GetSelectedIndex(), 1);
	TestTrue(TEXT("Choosing custom is a pending change."), ViewModel->HasPendingChanges());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelUnlistedValueTest,
	"PADO.UI.Settings.ViewModel.UnlistedValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelUnlistedValueTest::RunTest(const FString& Parameters)
{
	// 에디터 확장성 설정처럼 옵션 밖에서 정한 렌더링 해상도다.
	constexpr float UnlistedScale = 87.0f;

	UPDGameUserSettings* Settings = PDSettingsViewModelTests::MakeSettings();
	Settings->SetResolutionScaleValueEx(UnlistedScale);
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(Settings);
	UPDSettingChoiceViewModel* ResolutionScale = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::GraphicsResolutionScale);
	if (!TestNotNull(TEXT("The display tab has a resolution scale choice."), ResolutionScale))
	{
		return false;
	}

	const int32 ListedCount = ResolutionScale->GetOptions().Num() - 1;
	TestEqual(TEXT("An unlisted value is shown as the last choice."), ResolutionScale->GetSelectedIndex(), ListedCount);
	TestFalse(TEXT("The unlisted value has a label."), ResolutionScale->GetSelectedText().IsEmpty());

	// 선택지 순서: 자동, 100%, ...
	ResolutionScale->SelectOption(1);
	TestEqual(TEXT("Choosing a listed value removes the extra choice."), ResolutionScale->GetOptions().Num(), ListedCount);
	TestTrue(TEXT("The listed value is pending."), FMath::IsNearlyEqual(ViewModel->GetPendingValues().ResolutionScale, 100.0f));

	ViewModel->Revert();
	TestEqual(TEXT("Reverting shows the unlisted value again."), ResolutionScale->GetSelectedIndex(), ResolutionScale->GetOptions().Num() - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelDependentItemsTest,
	"PADO.UI.Settings.ViewModel.DependentItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelDependentItemsTest::RunTest(const FString& Parameters)
{
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(PDSettingsViewModelTests::MakeSettings());
	UPDSettingToggleViewModel* Subtitles = PDSettingsViewModelTests::FindItem<UPDSettingToggleViewModel>(ViewModel, PDSettingsText::Key::GeneralSubtitles);
	UPDSettingChoiceViewModel* SubtitleSize = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::GeneralSubtitleSize);
	UPDSettingChoiceViewModel* ColorVision = PDSettingsViewModelTests::FindItem<UPDSettingChoiceViewModel>(ViewModel, PDSettingsText::Key::AccessibilityColorVision);
	UPDSettingSliderViewModel* Severity = PDSettingsViewModelTests::FindItem<UPDSettingSliderViewModel>(ViewModel, PDSettingsText::Key::AccessibilityColorVisionSeverity);
	if (!TestNotNull(TEXT("Subtitles exist."), Subtitles) || !TestNotNull(TEXT("Subtitle size exists."), SubtitleSize)
		|| !TestNotNull(TEXT("Color vision exists."), ColorVision) || !TestNotNull(TEXT("Color vision severity exists."), Severity))
	{
		return false;
	}

	TestTrue(TEXT("Subtitle size is editable while subtitles are on."), SubtitleSize->IsEditable());
	Subtitles->SetIsOnFromUser(false);
	TestFalse(TEXT("Subtitle size is locked while subtitles are off."), SubtitleSize->IsEditable());

	TestFalse(TEXT("Correction strength is locked while color vision assistance is off."), Severity->IsEditable());
	ColorVision->SelectOption(1);
	TestEqual(TEXT("The first assistance choice is protanope."), ViewModel->GetPendingValues().ColorVisionDeficiency, EColorVisionDeficiency::Protanope);
	TestTrue(TEXT("Correction strength is editable with assistance on."), Severity->IsEditable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelResetTabTest,
	"PADO.UI.Settings.ViewModel.ResetTab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelResetTabTest::RunTest(const FString& Parameters)
{
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(PDSettingsViewModelTests::MakeSettings());
	UPDSettingSliderViewModel* Brightness = PDSettingsViewModelTests::FindItem<UPDSettingSliderViewModel>(ViewModel, PDSettingsText::Key::DisplayBrightness);
	UPDSettingSliderViewModel* MusicVolume = PDSettingsViewModelTests::FindItem<UPDSettingSliderViewModel>(ViewModel, PDSettingsText::Key::AudioMusic);
	UPDSettingToggleViewModel* InvertMouseY = PDSettingsViewModelTests::FindItem<UPDSettingToggleViewModel>(ViewModel, PDSettingsText::Key::ControlsInvertMouseY);
	if (!TestNotNull(TEXT("Brightness exists."), Brightness) || !TestNotNull(TEXT("Music volume exists."), MusicVolume) || !TestNotNull(TEXT("Invert mouse Y exists."), InvertMouseY))
	{
		return false;
	}

	Brightness->SetValueFromUser(20.0f);
	MusicVolume->SetValueFromUser(30.0f);
	InvertMouseY->SetIsOnFromUser(true);

	ViewModel->ResetTabToDefaults(EPDSettingsTab::Audio);
	TestEqual(TEXT("Resetting the audio tab restores music volume."), MusicVolume->GetValue(), 100.0f);
	TestEqual(TEXT("Resetting the audio tab keeps display changes."), Brightness->GetValue(), 20.0f);
	TestTrue(TEXT("Resetting the audio tab keeps control changes."), InvertMouseY->IsOn());
	TestTrue(TEXT("Other tabs still have pending changes."), ViewModel->HasPendingChanges());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelKeyBindingSwapTest,
	"PADO.UI.Settings.ViewModel.KeyBindingSwap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelKeyBindingSwapTest::RunTest(const FString& Parameters)
{
	const TArray<FPDKeyBindingEntry> KeyBindings =
	{
		PDSettingsViewModelTests::MakeKeyBinding(TEXT("Jump"), EKeys::SpaceBar),
		PDSettingsViewModelTests::MakeKeyBinding(TEXT("Interact"), EKeys::E),
	};
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(PDSettingsViewModelTests::MakeSettings(), KeyBindings);
	UPDSettingKeyBindingViewModel* Jump = PDSettingsViewModelTests::FindItem<UPDSettingKeyBindingViewModel>(ViewModel, TEXT("Jump"));
	UPDSettingKeyBindingViewModel* Interact = PDSettingsViewModelTests::FindItem<UPDSettingKeyBindingViewModel>(ViewModel, TEXT("Interact"));
	if (!TestNotNull(TEXT("Jump has a key row."), Jump) || !TestNotNull(TEXT("Interact has a key row."), Interact))
	{
		return false;
	}

	TestEqual(TEXT("A key used by another action is reported."), ViewModel->FindKeyConflict(TEXT("Jump"), EKeys::E), FName(TEXT("Interact")));
	TestTrue(TEXT("A free key has no conflict."), ViewModel->FindKeyConflict(TEXT("Jump"), EKeys::F).IsNone());

	ViewModel->AssignKey(TEXT("Jump"), EKeys::E);
	TestEqual(TEXT("Jump takes the new key."), Jump->GetKey(), EKeys::E);
	TestEqual(TEXT("The other action takes the old key."), Interact->GetKey(), EKeys::SpaceBar);
	TestTrue(TEXT("Changing a key creates a pending change."), ViewModel->HasPendingChanges());

	ViewModel->ResetTabToDefaults(EPDSettingsTab::KeyBindings);
	TestEqual(TEXT("Resetting key bindings restores default keys."), Jump->GetKey(), EKeys::SpaceBar);
	TestFalse(TEXT("Restored keys leave nothing to apply."), ViewModel->HasPendingChanges());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelNoKeyBindingsTest,
	"PADO.UI.Settings.ViewModel.NoKeyBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelNoKeyBindingsTest::RunTest(const FString& Parameters)
{
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(PDSettingsViewModelTests::MakeSettings());
	const TArray<UPDSettingItemViewModel*> KeyBindingItems = ViewModel->GetItems(EPDSettingsTab::KeyBindings);

	TestEqual(TEXT("Without mappable keys the tab shows one notice row."), KeyBindingItems.Num(), 1);
	TestTrue(TEXT("The notice row is a header."), KeyBindingItems.Num() == 1 && KeyBindingItems[0]->IsA<UPDSettingHeaderViewModel>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsViewModelNoSettingsTest,
	"PADO.UI.Settings.ViewModel.NoSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsViewModelNoSettingsTest::RunTest(const FString& Parameters)
{
	AddExpectedMessage(TEXT("could not find PDGameUserSettings"), EAutomationExpectedMessageFlags::Contains, 1);
	UPDSettingsViewModel* ViewModel = PDSettingsViewModelTests::MakeViewModel(nullptr);
	UPDSettingSliderViewModel* MasterVolume = PDSettingsViewModelTests::FindItem<UPDSettingSliderViewModel>(ViewModel, PDSettingsText::Key::AudioMaster);
	if (!TestNotNull(TEXT("Items are still built without settings."), MasterVolume))
	{
		return false;
	}

	TestFalse(TEXT("The screen reports settings as unavailable."), ViewModel->IsAvailable());
	TestFalse(TEXT("Items cannot be edited without settings."), MasterVolume->IsEditable());

	MasterVolume->SetValueFromUser(10.0f);
	TestFalse(TEXT("Edits are ignored without settings."), ViewModel->HasPendingChanges());
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPDSettingsDesignerPreviewTest,
	"PADO.UI.Settings.DesignerPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPDSettingsDesignerPreviewTest::RunTest(const FString& Parameters)
{
	const UPDSettingsViewModel* ViewModel = PDSettingsDesignerPreview::CreateViewModel(GetTransientPackage());
	TestTrue(TEXT("The preview uses settings that can be shown as editable."), ViewModel->IsAvailable());
	TestFalse(TEXT("The preview starts with nothing to apply."), ViewModel->HasPendingChanges());

	for (const EPDSettingsTab Tab : TEnumRange<EPDSettingsTab>())
	{
		TestTrue(FString::Printf(TEXT("Tab %s has preview rows."), *UEnum::GetValueAsString(Tab)), !ViewModel->GetItems(Tab).IsEmpty());
	}

	const TArray<UPDSettingItemViewModel*> KeyBindingItems = ViewModel->GetItems(EPDSettingsTab::KeyBindings);
	TestTrue(TEXT("The key bindings preview shows example keys instead of the empty notice."),
		!KeyBindingItems.IsEmpty() && KeyBindingItems[0]->IsA<UPDSettingKeyBindingViewModel>());
	return true;
}
#endif

#endif
