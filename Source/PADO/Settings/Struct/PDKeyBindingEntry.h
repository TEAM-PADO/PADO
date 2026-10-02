// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/** 키 설정 탭의 한 줄에 해당하는 키보드·마우스 매핑이다. */
struct FPDKeyBindingEntry
{
	/** Player Mappable Key Settings의 이름이다. 줄 식별자로 쓴다. */
	FName MappingName;

	/** 입력 에셋에 지정된 표시 이름이다. 비어 있으면 매핑 이름을 쓴다. */
	FText DisplayName;

	FKey CurrentKey;
	FKey DefaultKey;
};
