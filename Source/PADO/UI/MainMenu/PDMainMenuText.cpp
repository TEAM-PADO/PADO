// Copyright PADO. All Rights Reserved.

#include "PADO/UI/MainMenu/PDMainMenuText.h"

FText PDMainMenuText::Get(const TCHAR* InKey)
{
	return FText::FromStringTable(FName(TableId), FString(InKey));
}
