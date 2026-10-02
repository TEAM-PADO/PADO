// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "InputCoreTypes.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/UI/Settings/Enum/PDSettingsTab.h"

/**
 * 옵션 창에서 편집하는 설정 값 묶음이다.
 * 적용된 값과 편집 중인 값을 따로 들고 비교해 적용할 변경이 있는지 판단한다.
 * 키 설정은 GameUserSettings가 아니라 Enhanced Input 사용자 설정에 저장되므로 옵션 화면이 따로 채운다.
 */
struct PADO_API FPDSettingsValues
{
	/** 옵션에서 고를 수 있는 최고 품질 단계다(0 낮음 ~ 3 최상). */
	static constexpr int32 MaxSelectableQuality = UPDGameUserSettings::MaxQualityLevel;

	// 일반
	FString Culture;
	float UIScale = UPDGameUserSettings::DefaultUIScale;
	EPDPerformanceInfo PerformanceInfo = EPDPerformanceInfo::Off;
	bool bSubtitlesEnabled = true;
	EPDSubtitleSize SubtitleSize = EPDSubtitleSize::Medium;
	bool bSubtitleBackground = true;

	// 화면 (그래픽 포함)
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;
	FIntPoint Resolution = FIntPoint::ZeroValue;
	/** 0이면 무제한이다. */
	float FrameRateLimit = 0.0f;
	bool bVSyncEnabled = false;
	float Brightness = UPDGameUserSettings::DefaultBrightness;
	TMap<EPDGraphicsQualityGroup, int32> QualityLevels;
	/** 그래픽 품질에서 "사용자 지정"을 고른 상태다. 세부 품질을 하나라도 바꾸면 켜진다. */
	bool bCustomQuality = false;
	/** 렌더링 해상도(%)다. 0이면 프로젝트 기본값(자동)이다. */
	float ResolutionScale = 0.0f;
	bool bMotionBlurEnabled = true;
	float FieldOfView = UPDGameUserSettings::DefaultFieldOfView;

	// 소리
	TMap<EPDVolumeCategory, float> Volumes;

	// 조작
	float MouseSensitivity = UPDGameUserSettings::DefaultMouseSensitivity;
	float AimSensitivity = UPDGameUserSettings::DefaultAimSensitivity;
	bool bInvertMouseY = false;

	// 키 설정 (매핑 이름 → 키보드·마우스 키)
	TMap<FName, FKey> KeyBindings;

	// 접근성
	EColorVisionDeficiency ColorVisionDeficiency = EColorVisionDeficiency::NormalVision;
	int32 ColorVisionSeverity = UPDGameUserSettings::DefaultColorVisionSeverity;
	float CameraShakeScale = UPDGameUserSettings::DefaultCameraShakeScale;

	/** 설정 객체에서 값을 읽는다. 해상도가 비어 있으면 바탕 화면 해상도로 채운다. 키 설정은 채우지 않는다. */
	static FPDSettingsValues FromSettings(const UPDGameUserSettings& Settings);

	/** 설치 직후의 기본값이다. 키 설정은 채우지 않는다. */
	static FPDSettingsValues MakeDefaults();

	/** 설정 객체에 값을 쓴다. 엔진 적용과 저장은 호출한 쪽이 한다. 키 설정은 쓰지 않는다. */
	void ApplyTo(UPDGameUserSettings& Settings) const;

	/** 지정한 탭에 속한 값만 Source의 값으로 바꾼다. */
	void CopyTab(EPDSettingsTab Tab, const FPDSettingsValues& Source);

	float GetVolume(EPDVolumeCategory Category) const;
	void SetVolume(EPDVolumeCategory Category, float InVolume);

	int32 GetQualityLevel(EPDGraphicsQualityGroup Group) const;
	void SetQualityLevel(EPDGraphicsQualityGroup Group, int32 Level);

	/** 세부 품질이 모두 같으면 그 단계, 하나라도 다르면 -1(사용자 지정)이다. */
	int32 GetOverallQuality() const;

	/** 세부 품질을 모두 같은 단계로 바꾼다. */
	void SetOverallQuality(int32 Level);

	bool operator==(const FPDSettingsValues& Other) const;
	bool operator!=(const FPDSettingsValues& Other) const { return !(*this == Other); }
};
