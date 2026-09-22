#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PDSetByCallerValueStruct.generated.h"

/** GameplayEffect의 SetByCaller 자리에 넣을 태그와 기본 수치다. */
USTRUCT(BlueprintType)
struct PADO_API FPDSetByCallerValueStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	FGameplayTag DataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	float Magnitude = 0.0f;

	bool Validate(FString& OutError) const;
};
