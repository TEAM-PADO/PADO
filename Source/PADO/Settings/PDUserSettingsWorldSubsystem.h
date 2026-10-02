// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PDUserSettingsWorldSubsystem.generated.h"

class USoundMix;

/**
 * 개인 설정 중 월드마다 다시 걸어야 하는 값을 적용한다.
 * 맵을 열 때와 음량이 바뀔 때마다 분류별 Sound Class에 음량 덮어쓰기를 건다.
 * 에디터 PIE에서는 시작할 때 저장된 표시 설정(밝기·UI 크기·색각 보정·언어)을 걸고,
 * 마지막 PIE 월드가 끝나면 에디터 화면에 남지 않도록 되돌린다.
 */
UCLASS()
class PADO_API UPDUserSettingsWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 저장된(미리보기 중이면 미리보기) 음량을 이 월드의 오디오 장치에 적용한다. */
	void ApplyVolumes();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	FDelegateHandle VolumesChangedHandle;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> PushedSoundMix;

	bool bAppliedEditorPresentation = false;
};
