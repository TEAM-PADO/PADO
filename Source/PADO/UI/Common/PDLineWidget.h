// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Types/SlateEnums.h"
#include "PDLineWidget.generated.h"

class UImage;
class USizeBox;

/**
 * 화면 배율과 상관없이 1픽셀보다 얇아지지 않는 구분선이다.
 * 해상도가 1080p보다 낮으면 UI 배율이 1보다 작아져 1 단위 선이 1픽셀보다 얇아지고,
 * 픽셀에 맞추는 과정에서 위치에 따라 사라진다. 그래서 실제 화면에서 최소 1픽셀이 되도록 두께를 늘린다.
 * 방향·두께·길이·색은 배치한 인스턴스의 디테일에서 정한다.
 */
UCLASS(Abstract, Blueprintable)
class PADO_API UPDLineWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 선 방향이다. 세로선은 너비가, 가로선은 높이가 두께다. */
	UPROPERTY(EditAnywhere, Category = "PADO|Line")
	TEnumAsByte<EOrientation> Orientation = Orient_Vertical;

	/** 배율 1에서의 두께다. 실제 화면에서는 1픽셀보다 얇아지지 않는다. */
	UPROPERTY(EditAnywhere, Category = "PADO|Line", meta = (ClampMin = "0.1"))
	float Thickness = 1.0f;

	/** 선 길이다. 0이면 놓인 칸을 가득 채운다. */
	UPROPERTY(EditAnywhere, Category = "PADO|Line", meta = (ClampMin = "0.0"))
	float Length = 0.0f;

	UPROPERTY(EditAnywhere, Category = "PADO|Line")
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.25f);

private:
	void ApplyLength();
	void ApplyThickness(float InThickness);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Line;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Line;

	float AppliedThickness = 0.0f;
};
