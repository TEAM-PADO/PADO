// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/Widget/PDSettingRowFrameWidget.h"

#include "CommonTextBlock.h"

void UPDSettingRowFrameWidget::SetLabel(const FText& InLabel)
{
	Text_Label->SetText(InLabel);
}
