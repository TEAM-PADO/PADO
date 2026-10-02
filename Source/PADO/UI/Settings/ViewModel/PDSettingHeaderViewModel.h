// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/UI/Settings/ViewModel/PDSettingItemViewModel.h"
#include "PDSettingHeaderViewModel.generated.h"

/** 값 없이 이름만 보여 주는 줄이다. 탭 안 소제목(예: 세부 품질)과 빈 목록 안내에 쓴다. */
UCLASS(BlueprintType)
class PADO_API UPDSettingHeaderViewModel : public UPDSettingItemViewModel
{
	GENERATED_BODY()
};
