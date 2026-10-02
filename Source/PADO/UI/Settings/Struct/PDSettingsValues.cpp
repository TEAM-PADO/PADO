// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Struct/PDSettingsValues.h"

#include "UObject/Package.h"

namespace PDSettingsValues
{
	/** 슬라이더 눈금 계산에서 생기는 오차를 같은 값으로 본다. */
	constexpr float ValueTolerance = 0.001f;

	bool IsNearlyEqual(const float A, const float B)
	{
		return FMath::IsNearlyEqual(A, B, ValueTolerance);
	}
}

FPDSettingsValues FPDSettingsValues::FromSettings(const UPDGameUserSettings& Settings)
{
	FPDSettingsValues Values;
	Values.WindowMode = Settings.GetFullscreenMode();
	Values.Resolution = Settings.GetScreenResolution();
	if (Values.Resolution.X <= 0 || Values.Resolution.Y <= 0)
	{
		Values.Resolution = Settings.GetDesktopResolution();
	}

	Values.FrameRateLimit = Settings.GetFrameRateLimit();
	Values.bVSyncEnabled = Settings.IsVSyncEnabled();
	Values.Brightness = Settings.GetBrightness();

	for (const EPDGraphicsQualityGroup Group : TEnumRange<EPDGraphicsQualityGroup>())
	{
		Values.SetQualityLevel(Group, Settings.GetQualityGroupLevel(Group));
	}
	Values.bCustomQuality = Settings.IsCustomGraphicsQuality();
	Values.ResolutionScale = Settings.GetResolutionScaleValue();
	Values.bMotionBlurEnabled = Settings.IsMotionBlurEnabled();
	Values.FieldOfView = Settings.GetFieldOfView();

	for (const EPDVolumeCategory Category : TEnumRange<EPDVolumeCategory>())
	{
		Values.Volumes.Add(Category, Settings.GetVolume(Category));
	}

	Values.MouseSensitivity = Settings.GetMouseSensitivity();
	Values.AimSensitivity = Settings.GetAimSensitivity();
	Values.bInvertMouseY = Settings.IsMouseYInverted();

	Values.UIScale = Settings.GetUIScale();
	Values.PerformanceInfo = Settings.GetPerformanceInfo();
	Values.bSubtitlesEnabled = Settings.AreSubtitlesEnabled();
	Values.SubtitleSize = Settings.GetSubtitleSize();
	Values.bSubtitleBackground = Settings.IsSubtitleBackgroundEnabled();
	Values.Culture = Settings.GetCulture();

	Values.ColorVisionDeficiency = Settings.GetColorVisionDeficiency();
	Values.ColorVisionSeverity = Settings.GetColorVisionSeverity();
	Values.CameraShakeScale = Settings.GetCameraShakeScale();
	return Values;
}

FPDSettingsValues FPDSettingsValues::MakeDefaults()
{
	// 사용 중인 설정 객체를 건드리지 않도록 임시 객체에서 기본값을 읽는다.
	UPDGameUserSettings* DefaultSettings = NewObject<UPDGameUserSettings>(GetTransientPackage());
	DefaultSettings->SetToDefaults();
	return FromSettings(*DefaultSettings);
}

void FPDSettingsValues::ApplyTo(UPDGameUserSettings& Settings) const
{
	Settings.SetFullscreenMode(WindowMode);
	if (Resolution.X > 0 && Resolution.Y > 0)
	{
		Settings.SetScreenResolution(Resolution);
	}

	Settings.SetFrameRateLimit(FrameRateLimit);
	Settings.SetVSyncEnabled(bVSyncEnabled);
	Settings.SetBrightness(Brightness);

	for (const EPDGraphicsQualityGroup Group : TEnumRange<EPDGraphicsQualityGroup>())
	{
		Settings.SetQualityGroupLevel(Group, GetQualityLevel(Group));
	}

	// 옵션에 없는 지형 품질은 품질 단계 하나로 맞춰져 있을 때만 따라간다.
	const int32 OverallQuality = GetOverallQuality();
	if (OverallQuality >= 0)
	{
		Settings.SetLandscapeQuality(OverallQuality);
	}
	Settings.SetCustomGraphicsQuality(bCustomQuality);

	// 해상도와 화면 모드를 정한 뒤에 렌더링 해상도를 쓴다.
	Settings.SetResolutionScaleValueEx(ResolutionScale);
	Settings.SetMotionBlurEnabled(bMotionBlurEnabled);
	Settings.SetFieldOfView(FieldOfView);

	for (const EPDVolumeCategory Category : TEnumRange<EPDVolumeCategory>())
	{
		Settings.SetVolume(Category, GetVolume(Category));
	}

	Settings.SetMouseSensitivity(MouseSensitivity);
	Settings.SetAimSensitivity(AimSensitivity);
	Settings.SetMouseYInverted(bInvertMouseY);

	Settings.SetUIScale(UIScale);
	Settings.SetPerformanceInfo(PerformanceInfo);
	Settings.SetSubtitlesEnabled(bSubtitlesEnabled);
	Settings.SetSubtitleSize(SubtitleSize);
	Settings.SetSubtitleBackgroundEnabled(bSubtitleBackground);
	Settings.SetCulture(Culture);

	Settings.SetColorVisionDeficiency(ColorVisionDeficiency);
	Settings.SetColorVisionSeverity(ColorVisionSeverity);
	Settings.SetCameraShakeScale(CameraShakeScale);
}

void FPDSettingsValues::CopyTab(const EPDSettingsTab Tab, const FPDSettingsValues& Source)
{
	switch (Tab)
	{
	case EPDSettingsTab::General:
		Culture = Source.Culture;
		UIScale = Source.UIScale;
		PerformanceInfo = Source.PerformanceInfo;
		bSubtitlesEnabled = Source.bSubtitlesEnabled;
		SubtitleSize = Source.SubtitleSize;
		bSubtitleBackground = Source.bSubtitleBackground;
		break;

	case EPDSettingsTab::Display:
		WindowMode = Source.WindowMode;
		Resolution = Source.Resolution;
		FrameRateLimit = Source.FrameRateLimit;
		bVSyncEnabled = Source.bVSyncEnabled;
		Brightness = Source.Brightness;
		QualityLevels = Source.QualityLevels;
		bCustomQuality = Source.bCustomQuality;
		ResolutionScale = Source.ResolutionScale;
		bMotionBlurEnabled = Source.bMotionBlurEnabled;
		FieldOfView = Source.FieldOfView;
		break;

	case EPDSettingsTab::Audio:
		Volumes = Source.Volumes;
		break;

	case EPDSettingsTab::Controls:
		MouseSensitivity = Source.MouseSensitivity;
		AimSensitivity = Source.AimSensitivity;
		bInvertMouseY = Source.bInvertMouseY;
		break;

	case EPDSettingsTab::KeyBindings:
		KeyBindings = Source.KeyBindings;
		break;

	case EPDSettingsTab::Accessibility:
		ColorVisionDeficiency = Source.ColorVisionDeficiency;
		ColorVisionSeverity = Source.ColorVisionSeverity;
		CameraShakeScale = Source.CameraShakeScale;
		break;
	}
}

float FPDSettingsValues::GetVolume(const EPDVolumeCategory Category) const
{
	const float* Volume = Volumes.Find(Category);
	return Volume ? *Volume : UPDGameUserSettings::DefaultVolume;
}

void FPDSettingsValues::SetVolume(const EPDVolumeCategory Category, const float InVolume)
{
	Volumes.Add(Category, FMath::Clamp(InVolume, 0.0f, 1.0f));
}

int32 FPDSettingsValues::GetQualityLevel(const EPDGraphicsQualityGroup Group) const
{
	const int32* Level = QualityLevels.Find(Group);
	return Level ? *Level : MaxSelectableQuality;
}

void FPDSettingsValues::SetQualityLevel(const EPDGraphicsQualityGroup Group, const int32 Level)
{
	QualityLevels.Add(Group, FMath::Clamp(Level, 0, MaxSelectableQuality));
}

int32 FPDSettingsValues::GetOverallQuality() const
{
	int32 OverallQuality = INDEX_NONE;
	for (const EPDGraphicsQualityGroup Group : TEnumRange<EPDGraphicsQualityGroup>())
	{
		const int32 Level = GetQualityLevel(Group);
		if (OverallQuality == INDEX_NONE)
		{
			OverallQuality = Level;
		}
		else if (OverallQuality != Level)
		{
			return INDEX_NONE;
		}
	}

	return OverallQuality;
}

void FPDSettingsValues::SetOverallQuality(const int32 Level)
{
	for (const EPDGraphicsQualityGroup Group : TEnumRange<EPDGraphicsQualityGroup>())
	{
		SetQualityLevel(Group, Level);
	}
}

bool FPDSettingsValues::operator==(const FPDSettingsValues& Other) const
{
	using PDSettingsValues::IsNearlyEqual;

	if (WindowMode != Other.WindowMode
		|| Resolution != Other.Resolution
		|| !IsNearlyEqual(FrameRateLimit, Other.FrameRateLimit)
		|| bVSyncEnabled != Other.bVSyncEnabled
		|| !IsNearlyEqual(Brightness, Other.Brightness)
		|| bCustomQuality != Other.bCustomQuality
		|| !IsNearlyEqual(ResolutionScale, Other.ResolutionScale)
		|| bMotionBlurEnabled != Other.bMotionBlurEnabled
		|| !IsNearlyEqual(FieldOfView, Other.FieldOfView)
		|| !IsNearlyEqual(MouseSensitivity, Other.MouseSensitivity)
		|| !IsNearlyEqual(AimSensitivity, Other.AimSensitivity)
		|| bInvertMouseY != Other.bInvertMouseY
		|| !KeyBindings.OrderIndependentCompareEqual(Other.KeyBindings)
		|| !IsNearlyEqual(UIScale, Other.UIScale)
		|| PerformanceInfo != Other.PerformanceInfo
		|| bSubtitlesEnabled != Other.bSubtitlesEnabled
		|| SubtitleSize != Other.SubtitleSize
		|| bSubtitleBackground != Other.bSubtitleBackground
		|| Culture != Other.Culture
		|| ColorVisionDeficiency != Other.ColorVisionDeficiency
		|| ColorVisionSeverity != Other.ColorVisionSeverity
		|| !IsNearlyEqual(CameraShakeScale, Other.CameraShakeScale))
	{
		return false;
	}

	for (const EPDGraphicsQualityGroup Group : TEnumRange<EPDGraphicsQualityGroup>())
	{
		if (GetQualityLevel(Group) != Other.GetQualityLevel(Group))
		{
			return false;
		}
	}

	for (const EPDVolumeCategory Category : TEnumRange<EPDVolumeCategory>())
	{
		if (!IsNearlyEqual(GetVolume(Category), Other.GetVolume(Category)))
		{
			return false;
		}
	}

	return true;
}
