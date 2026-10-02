// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/PDSettingsText.h"

#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"

FText PDSettingsText::Get(const TCHAR* InKey)
{
	return FText::FromStringTable(FName(TableId), FString(InKey));
}

FText PDSettingsText::GetOrFallback(const FString& InKey, const FText& Fallback)
{
	// 다른 문구 조회로 표가 이미 올라와 있다. 표가 없으면 이름 대신 Fallback을 쓴다.
	const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(FName(TableId));
	if (Table.IsValid() && Table->FindEntry(InKey).IsValid())
	{
		return FText::FromStringTable(FName(TableId), InKey);
	}

	return Fallback;
}
