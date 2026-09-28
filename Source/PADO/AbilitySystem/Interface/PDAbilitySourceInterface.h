#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDAbilitySourceInterface.generated.h"

class UPDAbilityDefinition;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPDAbilitySourceInterface : public UInterface
{
	GENERATED_BODY()
};

/** 아이템, 함정 등 구체 타입과 무관하게 불변 Ability Definition을 제공하는 계약이다. */
class PADO_API IPDAbilitySourceInterface
{
	GENERATED_BODY()

public:
	virtual bool ResolveAbilityDefinition(
		const UPDAbilityDefinition*& OutDefinition) const = 0;
};
