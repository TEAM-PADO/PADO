#pragma once

#include "CoreMinimal.h"
#include "PDItemMagazineConfig.generated.h"

/** 아이템 Definition에서 선택적으로 조립하는 탄창 설정이다. */
USTRUCT(BlueprintType)
struct PADO_API FPDItemMagazineConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magazine")
	bool bEnabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magazine",
		meta = (EditCondition = "bEnabled", ClampMin = "1"))
	int32 Capacity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magazine",
		meta = (EditCondition = "bEnabled", ClampMin = "0.001"))
	float ReloadDuration = 1.0f;
};
