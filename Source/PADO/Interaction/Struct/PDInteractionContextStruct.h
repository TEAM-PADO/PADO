#pragma once

#include "CoreMinimal.h"
#include "PDInteractionContextStruct.generated.h"

class APDCharacterBase;
class UPrimitiveComponent;

/** 상호작용 한 번의 문맥이다. 대상은 이 값으로 누가 어디를 향해 요청했는지 안다. */
USTRUCT(BlueprintType)
struct PADO_API FPDInteractionContextStruct
{
	GENERATED_BODY()

	/** 상호작용하는 몸이다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Interaction")
	TObjectPtr<APDCharacterBase> Instigator = nullptr;

	/**
	 * 주체가 조준한 대상의 컴포넌트다. 차량의 문처럼 대상 안의 위치를 가릴 때 쓴다.
	 * 비어 있을 수 있다. 복제로 식별되지 않는 컴포넌트는 서버에 null로 도착하므로
	 * 대상은 이 값이 없어도 동작해야 한다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Interaction")
	TObjectPtr<UPrimitiveComponent> AimedComponent = nullptr;
};
