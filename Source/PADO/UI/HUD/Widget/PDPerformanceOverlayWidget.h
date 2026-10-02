// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PDPerformanceOverlayWidget.generated.h"

class UCommonTextBlock;

/**
 * 옵션의 "성능 정보 표시"에 따라 화면 구석에 FPS와 핑을 보여 준다.
 * 핑은 서버에 접속한 클라이언트일 때만 보인다. 혼자 하거나 방장일 때는 서버까지의 핑이 없다.
 * 입력은 받지 않으며 숫자는 일정 간격으로 평균을 내서 바꾼다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDPerformanceOverlayWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** 설정에서 표시 여부를 다시 읽는다. 설정을 적용할 때마다 호출된다. */
	void RefreshFromSettings();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateTexts(float AverageFps);
	bool IsConnectedToServer() const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Fps;

	/** 선택 요소다. 없으면 "FPS + 핑"을 골라도 FPS만 보인다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Ping;

	int32 FrameCount = 0;
	float ElapsedTime = 0.0f;
	bool bShowPing = false;
};
