// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "PADO/Settings/Enum/PDGraphicsQualityGroup.h"
#include "PADO/Settings/Enum/PDPerformanceInfo.h"
#include "PADO/Settings/Enum/PDSubtitleSize.h"
#include "PADO/Settings/Enum/PDVolumeCategory.h"
#include "Rendering/RenderingCommon.h"
#include "PDGameUserSettings.generated.h"

/**
 * PADO의 개인 환경 설정이다. 엔진 설정(화면 모드·해상도·프레임 제한·그래픽 품질·수직 동기화)에
 * 밝기·음량·조작·UI 표시·자막·접근성 값을 더한다.
 * 값은 플레이어 PC의 GameUserSettings.ini에만 저장되고 방이나 서버와 공유하지 않는다.
 * DefaultEngine.ini의 GameUserSettingsClassName으로 엔진이 이 클래스를 쓰게 한다.
 *
 * 시야각·조준 감도·화면 흔들림처럼 다른 파트가 적용하는 값은 getter만 공개한다.
 * 엔진에 직접 반영하는 값은 엔진이 쓰는 설정 객체(Get())일 때만 반영해, 테스트용 임시 객체가 화면을 바꾸지 않게 한다.
 */
UCLASS()
class PADO_API UPDGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxQualityLevel = 3;

	/** 설치 직후·기본값의 그래픽 품질 단계다(1 = 중간). */
	static constexpr int32 DefaultQualityLevel = 1;
	static constexpr float DefaultBrightness = 0.5f;
	static constexpr float DefaultVolume = 1.0f;

	static constexpr float DefaultMouseSensitivity = 1.0f;
	static constexpr float MinMouseSensitivity = 0.1f;
	static constexpr float MaxMouseSensitivity = 3.0f;
	static constexpr float DefaultAimSensitivity = 1.0f;
	static constexpr float MinAimSensitivity = 0.1f;
	static constexpr float MaxAimSensitivity = 2.0f;

	/** 3인칭 카메라의 기본 시야각이다. 플레이어 카메라의 평상시 시야각과 같다. */
	static constexpr float DefaultFieldOfView = 90.0f;
	static constexpr float MinFieldOfView = 70.0f;
	static constexpr float MaxFieldOfView = 110.0f;

	static constexpr float DefaultUIScale = 1.0f;
	static constexpr float MinUIScale = 0.8f;
	static constexpr float MaxUIScale = 1.3f;

	static constexpr int32 DefaultColorVisionSeverity = 5;
	static constexpr int32 MaxColorVisionSeverity = 10;
	static constexpr float DefaultCameraShakeScale = 1.0f;

	/** 엔진이 쓰는 설정 객체다. GameUserSettingsClassName이 이 클래스가 아니면 nullptr이다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings", meta = (DisplayName = "Get PD Game User Settings"))
	static UPDGameUserSettings* Get();

	/** 밝기 0~1을 엔진 표시 감마로 바꾼다. 0.5가 엔진 기본값 2.2다. */
	static float BrightnessToDisplayGamma(float InBrightness);

	virtual void SetToDefaults() override;
	virtual void ApplyNonResolutionSettings() override;

	// 화면
	float GetBrightness() const { return Brightness; }
	void SetBrightness(float InBrightness);

	// 그래픽
	int32 GetQualityGroupLevel(EPDGraphicsQualityGroup Group) const;
	void SetQualityGroupLevel(EPDGraphicsQualityGroup Group, int32 Level);

	/** 그래픽 품질을 "사용자 지정"으로 고른 상태인지다. 세부 품질이 모두 같아도 사용자 지정으로 보여 준다. */
	bool IsCustomGraphicsQuality() const { return bCustomGraphicsQuality; }
	void SetCustomGraphicsQuality(bool bInCustom);

	/** 렌더링 해상도(%)다. 0이면 프로젝트 기본 화면 비율을 쓴다. */
	float GetResolutionScaleValue() const;

	bool IsMotionBlurEnabled() const { return bMotionBlurEnabled; }
	void SetMotionBlurEnabled(bool bInEnabled);

	/** 3인칭 카메라 평상시 시야각이다. 플레이어 카메라가 읽어 적용한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	float GetFieldOfView() const { return FieldOfView; }

	/** 기본 시야각 대비 배율이다. 견착·조준처럼 자세마다 다른 시야각에 곱한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	float GetFieldOfViewScale() const { return FieldOfView / DefaultFieldOfView; }

	void SetFieldOfView(float InFieldOfView);

	// 소리
	/** 저장된 분류별 음량이다. 0~1이며 저장된 값이 없으면 1이다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	float GetVolume(EPDVolumeCategory Category) const;

	void SetVolume(EPDVolumeCategory Category, float InVolume);

	/** 미리보기 중이면 미리보기 음량, 아니면 저장된 음량이다. 실제 소리에 적용할 때 쓴다. */
	float GetEffectiveVolume(EPDVolumeCategory Category) const;

	// 조작
	/** 마우스 시점 회전에 곱할 배율이다. 시점 입력을 처리하는 쪽이 읽어 적용한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	void SetMouseSensitivity(float InSensitivity);

	/** 견착·조준 중 시점 회전에 추가로 곱할 배율이다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	float GetAimSensitivity() const { return AimSensitivity; }

	void SetAimSensitivity(float InSensitivity);

	/** 마우스 상하 시점 반전 여부다. 시점 입력을 처리하는 쪽이 읽어 적용한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	bool IsMouseYInverted() const { return bInvertMouseY; }

	void SetMouseYInverted(bool bInInvertMouseY);

	// 일반
	/** UI 전체 배율이다. 1이 기본 크기다. */
	float GetUIScale() const { return UIScale; }
	void SetUIScale(float InUIScale);

	/** 화면 구석에 보여 줄 성능 정보다. 성능 정보 오버레이가 읽는다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	EPDPerformanceInfo GetPerformanceInfo() const { return PerformanceInfo; }

	void SetPerformanceInfo(EPDPerformanceInfo InPerformanceInfo);

	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	bool AreSubtitlesEnabled() const { return bSubtitlesEnabled; }

	void SetSubtitlesEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	EPDSubtitleSize GetSubtitleSize() const { return SubtitleSize; }

	void SetSubtitleSize(EPDSubtitleSize InSubtitleSize);

	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	bool IsSubtitleBackgroundEnabled() const { return bSubtitleBackground; }

	void SetSubtitleBackgroundEnabled(bool bInEnabled);

	/** 표시 언어 문화권 코드(ko, en)다. 비어 있으면 실행할 때의 언어를 그대로 쓴다. */
	const FString& GetCulture() const { return Culture; }
	void SetCulture(const FString& InCulture);

	// 접근성
	EColorVisionDeficiency GetColorVisionDeficiency() const { return ColorVisionDeficiency; }
	void SetColorVisionDeficiency(EColorVisionDeficiency InDeficiency);

	/** 색각 보정 강도다. 0~10이다. */
	int32 GetColorVisionSeverity() const { return ColorVisionSeverity; }
	void SetColorVisionSeverity(int32 InSeverity);

	/** 화면 흔들림 배율이다. 카메라 셰이크를 시작하는 쪽이 Scale에 곱한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Settings")
	float GetCameraShakeScale() const { return CameraShakeScale; }

	void SetCameraShakeScale(float InScale);

	// 미리보기
	/** 저장하지 않고 밝기와 음량만 미리 반영한다. ClearPreview로 저장된 값으로 되돌린다. */
	void SetPreview(float InBrightness, const TMap<EPDVolumeCategory, float>& InVolumes);
	void ClearPreview();

	/** 밝기·UI 크기·색각 보정·언어처럼 화면 표시에 쓰는 설정을 엔진에 반영한다. */
	void ApplyPresentationSettings();

#if WITH_EDITOR
	/** PIE가 끝난 뒤 에디터 화면에 남은 표시 설정을 프로젝트 기본값으로 되돌린다. */
	void RestoreEditorPresentation();
#endif

	/** 음량을 적용하거나 미리보기 음량이 바뀐 직후 알린다. 월드마다 음량을 다시 거는 쪽이 구독한다. */
	FSimpleMulticastDelegate OnVolumesChanged;

	/** 설정을 엔진에 적용한 직후 알린다. 설정 값을 화면에 반영하는 UI(성능 정보 오버레이 등)가 구독한다. */
	FSimpleMulticastDelegate OnSettingsApplied;

private:
	/** 엔진이 쓰는 설정 객체인지 확인한다. 아니면 엔진에 반영하지 않는다. */
	bool IsActiveSettings() const;

	void ApplyDisplayGamma() const;
	void ApplyMotionBlur() const;
	void ApplyUIScale() const;
	void ApplySubtitles() const;
	void ApplyColorVision() const;
	void ApplyCulture() const;

	UPROPERTY(Config)
	float Brightness = DefaultBrightness;

	UPROPERTY(Config)
	bool bCustomGraphicsQuality = false;

	UPROPERTY(Config)
	bool bMotionBlurEnabled = true;

	UPROPERTY(Config)
	float FieldOfView = DefaultFieldOfView;

	UPROPERTY(Config)
	TMap<EPDVolumeCategory, float> Volumes;

	UPROPERTY(Config)
	float MouseSensitivity = DefaultMouseSensitivity;

	UPROPERTY(Config)
	float AimSensitivity = DefaultAimSensitivity;

	UPROPERTY(Config)
	bool bInvertMouseY = false;

	UPROPERTY(Config)
	float UIScale = DefaultUIScale;

	UPROPERTY(Config)
	EPDPerformanceInfo PerformanceInfo = EPDPerformanceInfo::Off;

	UPROPERTY(Config)
	bool bSubtitlesEnabled = true;

	UPROPERTY(Config)
	EPDSubtitleSize SubtitleSize = EPDSubtitleSize::Medium;

	UPROPERTY(Config)
	bool bSubtitleBackground = true;

	UPROPERTY(Config)
	FString Culture;

	UPROPERTY(Config)
	EColorVisionDeficiency ColorVisionDeficiency = EColorVisionDeficiency::NormalVision;

	UPROPERTY(Config)
	int32 ColorVisionSeverity = DefaultColorVisionSeverity;

	UPROPERTY(Config)
	float CameraShakeScale = DefaultCameraShakeScale;

	TOptional<float> PreviewBrightness;
	TMap<EPDVolumeCategory, float> PreviewVolumes;
};
