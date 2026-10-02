// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Common/PDLineWidget.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"

void UPDLineWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	Image_Line->SetColorAndOpacity(Color);
	ApplyLength();

	AppliedThickness = 0.0f;
	ApplyThickness(Thickness);
}

void UPDLineWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Scale은 레이아웃 1단위가 화면에서 몇 픽셀인지다. 1픽셀이 되는 두께를 최소값으로 둔다.
	const float PixelScale = MyGeometry.Scale;
	const float OnePixelThickness = PixelScale > KINDA_SMALL_NUMBER ? 1.0f / PixelScale : Thickness;
	ApplyThickness(FMath::Max(Thickness, OnePixelThickness));
}

void UPDLineWidget::ApplyLength()
{
	const bool bHasLength = Length > 0.0f;
	if (Orientation == Orient_Vertical)
	{
		if (bHasLength)
		{
			SizeBox_Line->SetHeightOverride(Length);
		}
		else
		{
			SizeBox_Line->ClearHeightOverride();
		}
	}
	else if (bHasLength)
	{
		SizeBox_Line->SetWidthOverride(Length);
	}
	else
	{
		SizeBox_Line->ClearWidthOverride();
	}
}

void UPDLineWidget::ApplyThickness(const float InThickness)
{
	if (FMath::IsNearlyEqual(InThickness, AppliedThickness))
	{
		return;
	}

	AppliedThickness = InThickness;
	if (Orientation == Orient_Vertical)
	{
		SizeBox_Line->SetWidthOverride(InThickness);
	}
	else
	{
		SizeBox_Line->SetHeightOverride(InThickness);
	}
}
