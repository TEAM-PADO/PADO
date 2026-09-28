#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PDActionHookStruct.generated.h"

/** GA의 의미 있는 실행 시점과 그 시점에 조립된 결과 Fragment 목록이다. */
USTRUCT(BlueprintType)
struct PADO_API FPDActionHookStruct
{
	GENERATED_BODY()

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Hook",
		meta = (Categories = "ActionHook"))
	FGameplayTag HookTag;

	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Hook")
	TArray<TObjectPtr<UPDActionFragment>> Fragments;

	bool Validate(FString& OutError) const;
	bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
};
