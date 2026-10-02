// Copyright PADO. All Rights Reserved.

#include "PADO/Settings/PDGameUserSettings.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/UserInterfaceSettings.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/GameplayStatics.h"

namespace PDGameUserSettings
{
	/** 밝기 0과 1에 대응하는 표시 감마다. 가운데(0.5)가 엔진 기본값 2.2가 되도록 맞췄다. */
	constexpr float MinDisplayGamma = 1.7f;
	constexpr float MaxDisplayGamma = 2.7f;

	/** 모션 블러를 끌 때 0으로, 켤 때 -1(후처리 설정 따름)로 둔다. */
	const TCHAR* const MotionBlurAmountCVarName = TEXT("r.MotionBlur.Amount");
	constexpr float MotionBlurFollowPostProcess = -1.0f;
	constexpr float MotionBlurOff = 0.0f;

	/** 옵션을 처음 적용하기 전 프로젝트 값이다. UI 크기는 이 값에 곱하고, 에디터는 PIE 뒤 이 값으로 되돌린다. */
	TOptional<float> ProjectApplicationScale;
	TOptional<float> ProjectDisplayGamma;

	float GetProjectApplicationScale()
	{
		if (!ProjectApplicationScale.IsSet())
		{
			ProjectApplicationScale = GetDefault<UUserInterfaceSettings>()->ApplicationScale;
		}

		return ProjectApplicationScale.GetValue();
	}

	float GetProjectDisplayGamma()
	{
		if (!ProjectDisplayGamma.IsSet())
		{
			ProjectDisplayGamma = GEngine ? GEngine->DisplayGamma : UPDGameUserSettings::BrightnessToDisplayGamma(UPDGameUserSettings::DefaultBrightness);
		}

		return ProjectDisplayGamma.GetValue();
	}

	void SetMotionBlurAmount(const float Amount)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(MotionBlurAmountCVarName))
		{
			CVar->Set(Amount, ECVF_SetByGameSetting);
		}
	}
}

UPDGameUserSettings* UPDGameUserSettings::Get()
{
	return GEngine ? Cast<UPDGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

float UPDGameUserSettings::BrightnessToDisplayGamma(const float InBrightness)
{
	return FMath::Lerp(PDGameUserSettings::MinDisplayGamma, PDGameUserSettings::MaxDisplayGamma, FMath::Clamp(InBrightness, 0.0f, 1.0f));
}

void UPDGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	// 엔진 기본(최상) 대신 중간 품질로 시작한다. 렌더링 해상도는 프로젝트 기본(자동)을 그대로 둔다.
	for (const EPDGraphicsQualityGroup Group : TEnumRange<EPDGraphicsQualityGroup>())
	{
		SetQualityGroupLevel(Group, DefaultQualityLevel);
	}
	SetLandscapeQuality(DefaultQualityLevel);
	bCustomGraphicsQuality = false;

	Brightness = DefaultBrightness;
	bMotionBlurEnabled = true;
	FieldOfView = DefaultFieldOfView;
	Volumes.Reset();
	MouseSensitivity = DefaultMouseSensitivity;
	AimSensitivity = DefaultAimSensitivity;
	bInvertMouseY = false;
	UIScale = DefaultUIScale;
	PerformanceInfo = EPDPerformanceInfo::Off;
	bSubtitlesEnabled = true;
	SubtitleSize = EPDSubtitleSize::Medium;
	bSubtitleBackground = true;
	Culture.Reset();
	ColorVisionDeficiency = EColorVisionDeficiency::NormalVision;
	ColorVisionSeverity = DefaultColorVisionSeverity;
	CameraShakeScale = DefaultCameraShakeScale;
}

void UPDGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	if (IsActiveSettings())
	{
		ApplyPresentationSettings();
		ApplyMotionBlur();
		ApplySubtitles();
	}

	OnVolumesChanged.Broadcast();
	OnSettingsApplied.Broadcast();
}

void UPDGameUserSettings::SetBrightness(const float InBrightness)
{
	Brightness = FMath::Clamp(InBrightness, 0.0f, 1.0f);
}

int32 UPDGameUserSettings::GetQualityGroupLevel(const EPDGraphicsQualityGroup Group) const
{
	switch (Group)
	{
	case EPDGraphicsQualityGroup::ViewDistance:
		return GetViewDistanceQuality();

	case EPDGraphicsQualityGroup::Shadow:
		return GetShadowQuality();

	case EPDGraphicsQualityGroup::GlobalIllumination:
		return GetGlobalIlluminationQuality();

	case EPDGraphicsQualityGroup::Reflection:
		return GetReflectionQuality();

	case EPDGraphicsQualityGroup::AntiAliasing:
		return GetAntiAliasingQuality();

	case EPDGraphicsQualityGroup::Texture:
		return GetTextureQuality();

	case EPDGraphicsQualityGroup::Effects:
		return GetVisualEffectQuality();

	case EPDGraphicsQualityGroup::PostProcess:
		return GetPostProcessingQuality();

	case EPDGraphicsQualityGroup::Foliage:
		return GetFoliageQuality();

	case EPDGraphicsQualityGroup::Shading:
		return GetShadingQuality();
	}

	return MaxQualityLevel;
}

void UPDGameUserSettings::SetQualityGroupLevel(const EPDGraphicsQualityGroup Group, const int32 Level)
{
	const int32 ClampedLevel = FMath::Clamp(Level, 0, MaxQualityLevel);
	switch (Group)
	{
	case EPDGraphicsQualityGroup::ViewDistance:
		SetViewDistanceQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::Shadow:
		SetShadowQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::GlobalIllumination:
		SetGlobalIlluminationQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::Reflection:
		SetReflectionQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::AntiAliasing:
		SetAntiAliasingQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::Texture:
		SetTextureQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::Effects:
		SetVisualEffectQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::PostProcess:
		SetPostProcessingQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::Foliage:
		SetFoliageQuality(ClampedLevel);
		break;

	case EPDGraphicsQualityGroup::Shading:
		SetShadingQuality(ClampedLevel);
		break;
	}
}

void UPDGameUserSettings::SetCustomGraphicsQuality(const bool bInCustom)
{
	bCustomGraphicsQuality = bInCustom;
}

float UPDGameUserSettings::GetResolutionScaleValue() const
{
	float ScaleNormalized = 0.0f;
	float ScaleValue = 0.0f;
	float MinScaleValue = 0.0f;
	float MaxScaleValue = 0.0f;
	GetResolutionScaleInformationEx(ScaleNormalized, ScaleValue, MinScaleValue, MaxScaleValue);
	return ScaleValue;
}

void UPDGameUserSettings::SetMotionBlurEnabled(const bool bInEnabled)
{
	bMotionBlurEnabled = bInEnabled;
}

void UPDGameUserSettings::SetFieldOfView(const float InFieldOfView)
{
	FieldOfView = FMath::Clamp(InFieldOfView, MinFieldOfView, MaxFieldOfView);
}

float UPDGameUserSettings::GetVolume(const EPDVolumeCategory Category) const
{
	const float* Volume = Volumes.Find(Category);
	return Volume ? *Volume : DefaultVolume;
}

void UPDGameUserSettings::SetVolume(const EPDVolumeCategory Category, const float InVolume)
{
	Volumes.Add(Category, FMath::Clamp(InVolume, 0.0f, 1.0f));
}

float UPDGameUserSettings::GetEffectiveVolume(const EPDVolumeCategory Category) const
{
	const float* PreviewVolume = PreviewVolumes.Find(Category);
	return PreviewVolume ? *PreviewVolume : GetVolume(Category);
}

void UPDGameUserSettings::SetMouseSensitivity(const float InSensitivity)
{
	MouseSensitivity = FMath::Clamp(InSensitivity, MinMouseSensitivity, MaxMouseSensitivity);
}

void UPDGameUserSettings::SetAimSensitivity(const float InSensitivity)
{
	AimSensitivity = FMath::Clamp(InSensitivity, MinAimSensitivity, MaxAimSensitivity);
}

void UPDGameUserSettings::SetMouseYInverted(const bool bInInvertMouseY)
{
	bInvertMouseY = bInInvertMouseY;
}

void UPDGameUserSettings::SetUIScale(const float InUIScale)
{
	UIScale = FMath::Clamp(InUIScale, MinUIScale, MaxUIScale);
}

void UPDGameUserSettings::SetPerformanceInfo(const EPDPerformanceInfo InPerformanceInfo)
{
	PerformanceInfo = InPerformanceInfo;
}

void UPDGameUserSettings::SetSubtitlesEnabled(const bool bInEnabled)
{
	bSubtitlesEnabled = bInEnabled;
}

void UPDGameUserSettings::SetSubtitleSize(const EPDSubtitleSize InSubtitleSize)
{
	SubtitleSize = InSubtitleSize;
}

void UPDGameUserSettings::SetSubtitleBackgroundEnabled(const bool bInEnabled)
{
	bSubtitleBackground = bInEnabled;
}

void UPDGameUserSettings::SetCulture(const FString& InCulture)
{
	Culture = InCulture;
}

void UPDGameUserSettings::SetColorVisionDeficiency(const EColorVisionDeficiency InDeficiency)
{
	ColorVisionDeficiency = InDeficiency;
}

void UPDGameUserSettings::SetColorVisionSeverity(const int32 InSeverity)
{
	ColorVisionSeverity = FMath::Clamp(InSeverity, 0, MaxColorVisionSeverity);
}

void UPDGameUserSettings::SetCameraShakeScale(const float InScale)
{
	CameraShakeScale = FMath::Clamp(InScale, 0.0f, 1.0f);
}

void UPDGameUserSettings::SetPreview(const float InBrightness, const TMap<EPDVolumeCategory, float>& InVolumes)
{
	PreviewBrightness = FMath::Clamp(InBrightness, 0.0f, 1.0f);
	PreviewVolumes.Reset();
	for (const TPair<EPDVolumeCategory, float>& Pair : InVolumes)
	{
		PreviewVolumes.Add(Pair.Key, FMath::Clamp(Pair.Value, 0.0f, 1.0f));
	}

	ApplyDisplayGamma();
	OnVolumesChanged.Broadcast();
}

void UPDGameUserSettings::ClearPreview()
{
	const bool bHadPreview = PreviewBrightness.IsSet() || !PreviewVolumes.IsEmpty();
	PreviewBrightness.Reset();
	PreviewVolumes.Reset();

	if (bHadPreview)
	{
		ApplyDisplayGamma();
		OnVolumesChanged.Broadcast();
	}
}

void UPDGameUserSettings::ApplyPresentationSettings()
{
	ApplyDisplayGamma();
	ApplyUIScale();
	ApplyColorVision();
	ApplyCulture();
}

#if WITH_EDITOR
void UPDGameUserSettings::RestoreEditorPresentation()
{
	if (!IsActiveSettings())
	{
		return;
	}

	if (GEngine)
	{
		GEngine->DisplayGamma = PDGameUserSettings::GetProjectDisplayGamma();
	}

	GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = PDGameUserSettings::GetProjectApplicationScale();
	UWidgetBlueprintLibrary::SetColorVisionDeficiencyType(EColorVisionDeficiency::NormalVision, 0.0f, false, false);
	PDGameUserSettings::SetMotionBlurAmount(PDGameUserSettings::MotionBlurFollowPostProcess);
	FTextLocalizationManager::Get().DisableGameLocalizationPreview();
}
#endif

bool UPDGameUserSettings::IsActiveSettings() const
{
	return Get() == this;
}

void UPDGameUserSettings::ApplyDisplayGamma() const
{
	if (IsActiveSettings() && GEngine)
	{
		// 되돌릴 값을 먼저 잡아 둔다.
		PDGameUserSettings::GetProjectDisplayGamma();
		GEngine->DisplayGamma = BrightnessToDisplayGamma(PreviewBrightness.Get(Brightness));
	}
}

void UPDGameUserSettings::ApplyMotionBlur() const
{
	if (IsActiveSettings())
	{
		PDGameUserSettings::SetMotionBlurAmount(bMotionBlurEnabled ? PDGameUserSettings::MotionBlurFollowPostProcess : PDGameUserSettings::MotionBlurOff);
	}
}

void UPDGameUserSettings::ApplyUIScale() const
{
	if (IsActiveSettings())
	{
		// 프로젝트 DPI 규칙 결과에 곱해지는 값이라 화면 크기가 바뀌지 않아도 바로 반영된다.
		GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = PDGameUserSettings::GetProjectApplicationScale() * UIScale;
	}
}

void UPDGameUserSettings::ApplySubtitles() const
{
	if (IsActiveSettings())
	{
		UGameplayStatics::SetSubtitlesEnabled(bSubtitlesEnabled);
	}
}

void UPDGameUserSettings::ApplyColorVision() const
{
	if (IsActiveSettings())
	{
		const bool bCorrect = ColorVisionDeficiency != EColorVisionDeficiency::NormalVision;
		UWidgetBlueprintLibrary::SetColorVisionDeficiencyType(ColorVisionDeficiency, static_cast<float>(ColorVisionSeverity), bCorrect, false);
	}
}

void UPDGameUserSettings::ApplyCulture() const
{
	if (!IsActiveSettings() || Culture.IsEmpty())
	{
		return;
	}

#if WITH_EDITOR
	if (GIsEditor)
	{
		// 에디터 언어는 그대로 두고 게임 문구만 미리보기로 바꾼다.
		FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
		return;
	}
#endif

	if (FInternationalization::Get().GetCurrentLanguage()->GetName() != Culture)
	{
		FInternationalization::Get().SetCurrentLanguageAndLocale(Culture);
	}
}
