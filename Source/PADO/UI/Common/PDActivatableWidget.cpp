// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Common/PDActivatableWidget.h"

#include "Input/UIActionBindingHandle.h"

TOptional<FUIInputConfig> UPDActivatableWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
