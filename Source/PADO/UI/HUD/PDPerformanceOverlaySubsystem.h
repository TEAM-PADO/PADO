// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PDPerformanceOverlaySubsystem.generated.h"

class UPDPerformanceOverlayWidget;

/**
 * 로컬 플레이어마다 성능 정보 오버레이를 화면에 띄운다.
 * 맵을 옮기면 화면 위젯이 모두 지워지므로 플레이어 컨트롤러가 바뀔 때마다 다시 만든다.
 * 표시 여부는 오버레이가 설정에서 읽고, 설정을 적용할 때마다 다시 읽게 한다.
 */
UCLASS()
class PADO_API UPDPerformanceOverlaySubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

private:
	void HandleSettingsApplied();

	UPROPERTY(Transient)
	TObjectPtr<UPDPerformanceOverlayWidget> OverlayWidget;

	FDelegateHandle SettingsAppliedHandle;
};
