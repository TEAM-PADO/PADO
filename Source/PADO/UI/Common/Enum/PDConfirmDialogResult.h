// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PDConfirmDialogResult.generated.h"

/** 공용 확인창을 닫은 이유다. 시간 초과와 뒤로가기는 취소로 본다. */
UENUM(BlueprintType)
enum class EPDConfirmDialogResult : uint8
{
	Confirmed	UMETA(DisplayName = "Confirmed"),
	Cancelled	UMETA(DisplayName = "Cancelled")
};
