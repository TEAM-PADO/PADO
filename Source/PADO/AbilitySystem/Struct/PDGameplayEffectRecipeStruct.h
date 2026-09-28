#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PADO/AbilitySystem/Struct/PDSetByCallerValueStruct.h"
#include "PDGameplayEffectRecipeStruct.generated.h"

class UGameplayEffect;

/** 하나의 GameplayEffectSpec을 만들기 위한 소스 독립적인 재료다. */
USTRUCT(BlueprintType)
struct PADO_API FPDGameplayEffectRecipeStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.0"))
	float EffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TArray<FPDSetByCallerValueStruct> SetByCallers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	FGameplayTagContainer DynamicGrantedTags;

	bool Validate(FString& OutError) const;
};
