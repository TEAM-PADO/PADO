// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 옵션 창 String Table의 경로와 항목 키다.
 * C++에서 만드는 문구는 모두 이 키로 조회해 로컬라이징 대상에 포함한다.
 * 항목 원본은 Docs/UI/Localization/ST_Settings.csv이며, 에디터에서 String Table 에셋으로 가져온다.
 * 옵션 항목의 이름 키는 항목 식별자로도 쓴다.
 */
namespace PDSettingsText
{
	/** 옵션 창 String Table 에셋 경로다. */
	inline constexpr const TCHAR* TableId = TEXT("/Game/PADO/UI/Localization/ST_Settings.ST_Settings");

	/** 키 설정 줄 이름 키의 접두사다. 매핑 이름을 붙여 찾고, 없으면 입력 에셋의 표시 이름을 쓴다. */
	inline constexpr const TCHAR* KeyBindingLabelPrefix = TEXT("Keybind.");

	namespace Key
	{
		inline constexpr const TCHAR* TabGeneral = TEXT("Tab.General");
		inline constexpr const TCHAR* TabDisplay = TEXT("Tab.Display");
		inline constexpr const TCHAR* TabAudio = TEXT("Tab.Audio");
		inline constexpr const TCHAR* TabControls = TEXT("Tab.Controls");
		inline constexpr const TCHAR* TabKeyBindings = TEXT("Tab.KeyBindings");
		inline constexpr const TCHAR* TabAccessibility = TEXT("Tab.Accessibility");

		inline constexpr const TCHAR* CommonOn = TEXT("Common.On");
		inline constexpr const TCHAR* CommonOff = TEXT("Common.Off");
		inline constexpr const TCHAR* CommonUnavailable = TEXT("Common.Unavailable");
		inline constexpr const TCHAR* CommonAuto = TEXT("Common.Auto");
		inline constexpr const TCHAR* CommonUnlimited = TEXT("Common.Unlimited");
		inline constexpr const TCHAR* CommonPercent = TEXT("Common.Percent");

		inline constexpr const TCHAR* DisplayWindowMode = TEXT("Display.WindowMode");
		inline constexpr const TCHAR* DisplayWindowModeFullscreen = TEXT("Display.WindowMode.Fullscreen");
		inline constexpr const TCHAR* DisplayWindowModeBorderless = TEXT("Display.WindowMode.Borderless");
		inline constexpr const TCHAR* DisplayWindowModeWindowed = TEXT("Display.WindowMode.Windowed");
		inline constexpr const TCHAR* DisplayResolution = TEXT("Display.Resolution");
		inline constexpr const TCHAR* DisplayResolutionValue = TEXT("Display.Resolution.Value");
		inline constexpr const TCHAR* DisplayResolutionLocked = TEXT("Display.Resolution.Locked");
		inline constexpr const TCHAR* DisplayFrameRateLimit = TEXT("Display.FrameRateLimit");
		inline constexpr const TCHAR* DisplayFrameRateLimitValue = TEXT("Display.FrameRateLimit.Value");
		inline constexpr const TCHAR* DisplayVSync = TEXT("Display.VSync");
		inline constexpr const TCHAR* DisplayBrightness = TEXT("Display.Brightness");

		inline constexpr const TCHAR* GraphicsQuality = TEXT("Graphics.Quality");
		inline constexpr const TCHAR* GraphicsQualityLow = TEXT("Graphics.Quality.Low");
		inline constexpr const TCHAR* GraphicsQualityMedium = TEXT("Graphics.Quality.Medium");
		inline constexpr const TCHAR* GraphicsQualityHigh = TEXT("Graphics.Quality.High");
		inline constexpr const TCHAR* GraphicsQualityEpic = TEXT("Graphics.Quality.Epic");
		inline constexpr const TCHAR* GraphicsQualityCustom = TEXT("Graphics.Quality.Custom");
		inline constexpr const TCHAR* GraphicsResolutionScale = TEXT("Graphics.ResolutionScale");
		inline constexpr const TCHAR* GraphicsMotionBlur = TEXT("Graphics.MotionBlur");
		inline constexpr const TCHAR* GraphicsFieldOfView = TEXT("Graphics.FieldOfView");
		inline constexpr const TCHAR* GraphicsDetail = TEXT("Graphics.Detail");
		inline constexpr const TCHAR* GraphicsViewDistance = TEXT("Graphics.ViewDistance");
		inline constexpr const TCHAR* GraphicsShadow = TEXT("Graphics.Shadow");
		inline constexpr const TCHAR* GraphicsGlobalIllumination = TEXT("Graphics.GlobalIllumination");
		inline constexpr const TCHAR* GraphicsReflection = TEXT("Graphics.Reflection");
		inline constexpr const TCHAR* GraphicsAntiAliasing = TEXT("Graphics.AntiAliasing");
		inline constexpr const TCHAR* GraphicsTexture = TEXT("Graphics.Texture");
		inline constexpr const TCHAR* GraphicsEffects = TEXT("Graphics.Effects");
		inline constexpr const TCHAR* GraphicsPostProcess = TEXT("Graphics.PostProcess");
		inline constexpr const TCHAR* GraphicsFoliage = TEXT("Graphics.Foliage");
		inline constexpr const TCHAR* GraphicsShading = TEXT("Graphics.Shading");

		inline constexpr const TCHAR* AudioMaster = TEXT("Audio.Master");
		inline constexpr const TCHAR* AudioMusic = TEXT("Audio.Music");
		inline constexpr const TCHAR* AudioEffects = TEXT("Audio.Effects");
		inline constexpr const TCHAR* AudioUI = TEXT("Audio.UI");

		inline constexpr const TCHAR* ControlsMouseSensitivity = TEXT("Controls.MouseSensitivity");
		inline constexpr const TCHAR* ControlsAimSensitivity = TEXT("Controls.AimSensitivity");
		inline constexpr const TCHAR* ControlsInvertMouseY = TEXT("Controls.InvertMouseY");

		inline constexpr const TCHAR* KeyBindingsEmpty = TEXT("KeyBindings.Empty");
		inline constexpr const TCHAR* KeyBindingsUnbound = TEXT("KeyBindings.Unbound");
		inline constexpr const TCHAR* KeyBindingsCapturePrompt = TEXT("KeyBindings.CapturePrompt");
		inline constexpr const TCHAR* KeyBindingsConflictTitle = TEXT("KeyBindings.ConflictTitle");
		inline constexpr const TCHAR* KeyBindingsConflictBody = TEXT("KeyBindings.ConflictBody");
		inline constexpr const TCHAR* KeyBindingsConflictConfirm = TEXT("KeyBindings.ConflictConfirm");

		inline constexpr const TCHAR* GeneralLanguage = TEXT("General.Language");
		inline constexpr const TCHAR* GeneralDisplayHeader = TEXT("General.DisplayHeader");
		inline constexpr const TCHAR* GeneralUIScale = TEXT("General.UIScale");
		inline constexpr const TCHAR* GeneralPerformanceInfo = TEXT("General.PerformanceInfo");
		inline constexpr const TCHAR* GeneralPerformanceInfoFps = TEXT("General.PerformanceInfo.Fps");
		inline constexpr const TCHAR* GeneralPerformanceInfoFpsAndPing = TEXT("General.PerformanceInfo.FpsAndPing");
		inline constexpr const TCHAR* GeneralSubtitlesHeader = TEXT("General.SubtitlesHeader");
		inline constexpr const TCHAR* GeneralSubtitles = TEXT("General.Subtitles");
		inline constexpr const TCHAR* GeneralSubtitleSize = TEXT("General.SubtitleSize");
		inline constexpr const TCHAR* GeneralSubtitleSizeSmall = TEXT("General.SubtitleSize.Small");
		inline constexpr const TCHAR* GeneralSubtitleSizeMedium = TEXT("General.SubtitleSize.Medium");
		inline constexpr const TCHAR* GeneralSubtitleSizeLarge = TEXT("General.SubtitleSize.Large");
		inline constexpr const TCHAR* GeneralSubtitleBackground = TEXT("General.SubtitleBackground");
		inline constexpr const TCHAR* GeneralSubtitleLocked = TEXT("General.SubtitleLocked");

		/** 성능 정보 오버레이 문구다. {Value}에 숫자가 들어간다. */
		inline constexpr const TCHAR* PerformanceInfoFpsValue = TEXT("PerformanceInfo.FpsValue");
		inline constexpr const TCHAR* PerformanceInfoPingValue = TEXT("PerformanceInfo.PingValue");

		inline constexpr const TCHAR* AccessibilityColorVision = TEXT("Accessibility.ColorVision");
		inline constexpr const TCHAR* AccessibilityColorVisionProtanope = TEXT("Accessibility.ColorVision.Protanope");
		inline constexpr const TCHAR* AccessibilityColorVisionDeuteranope = TEXT("Accessibility.ColorVision.Deuteranope");
		inline constexpr const TCHAR* AccessibilityColorVisionTritanope = TEXT("Accessibility.ColorVision.Tritanope");
		inline constexpr const TCHAR* AccessibilityColorVisionSeverity = TEXT("Accessibility.ColorVisionSeverity");
		inline constexpr const TCHAR* AccessibilityColorVisionLocked = TEXT("Accessibility.ColorVisionLocked");
		inline constexpr const TCHAR* AccessibilityCameraShake = TEXT("Accessibility.CameraShake");

		inline constexpr const TCHAR* DialogUnsavedTitle = TEXT("Dialog.UnsavedTitle");
		inline constexpr const TCHAR* DialogUnsavedBody = TEXT("Dialog.UnsavedBody");
		inline constexpr const TCHAR* DialogUnsavedConfirm = TEXT("Dialog.UnsavedConfirm");
		inline constexpr const TCHAR* DialogUnsavedCancel = TEXT("Dialog.UnsavedCancel");
		inline constexpr const TCHAR* DialogCancel = TEXT("Dialog.Cancel");
	}

	/** 형식 문구에 넘기는 인자 이름이다. String Table 원문의 {이름}과 같아야 한다. */
	namespace Arg
	{
		inline constexpr const TCHAR* Width = TEXT("Width");
		inline constexpr const TCHAR* Height = TEXT("Height");
		inline constexpr const TCHAR* Value = TEXT("Value");
		inline constexpr const TCHAR* Action = TEXT("Action");
		inline constexpr const TCHAR* KeyName = TEXT("Key");
	}

	/** 옵션 창 String Table 항목을 조회한다. */
	PADO_API FText Get(const TCHAR* InKey);

	/** 항목이 있으면 그 문구를, 없으면 Fallback을 반환한다. 입력 에셋에서 오는 이름처럼 키가 없을 수 있는 문구에 쓴다. */
	PADO_API FText GetOrFallback(const FString& InKey, const FText& Fallback);
}
