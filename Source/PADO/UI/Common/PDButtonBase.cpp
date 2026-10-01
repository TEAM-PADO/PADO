// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Common/PDButtonBase.h"

#include "CommonTextBlock.h"

void UPDButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;
	RefreshButtonText();
}

void UPDButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshButtonText();
}

void UPDButtonBase::RefreshButtonText()
{
	if (Text_Label)
	{
		Text_Label->SetText(ButtonText);
	}
}
