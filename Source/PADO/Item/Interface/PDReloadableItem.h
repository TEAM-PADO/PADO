#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDReloadableItem.generated.h"

UINTERFACE(BlueprintType)
class PADO_API UPDReloadableItem : public UInterface
{
	GENERATED_BODY()
};

/** Held Item 계층이 구체 무기 타입 없이 재장전을 요청하는 계약이다. */
class PADO_API IPDReloadableItem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item|Weapon")
	bool TryStartReload(AActor* RequestingHolder);
};
