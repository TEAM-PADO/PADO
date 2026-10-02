#pragma once

#include "CoreMinimal.h"
#include "PADO/Interaction/Struct/PDInteractionContextStruct.h"
#include "UObject/Interface.h"
#include "PDInteractable.generated.h"

UINTERFACE(BlueprintType)
class PADO_API UPDInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 상호작용 대상의 계약이다.
 *
 * 주체는 대상을 고르고 요청만 한다. 무엇을 할지, 지금 할 수 있는지는 대상이
 * 정한다. 손 닿는 거리는 주체의 속성이라 대상은 거리를 판단하지 않는다.
 */
class PADO_API IPDInteractable
{
	GENERATED_BODY()

public:
	/**
	 * 지금 이 주체와 상호작용할 수 있는지다. 상태만 본다.
	 * 로컬에서 대상을 고를 때와 서버가 요청을 확정할 때 모두 불린다.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PD|Interaction")
	bool CanInteract(const FPDInteractionContextStruct& Context) const;

	/** 서버에서만 불린다. 대상이 자기 로직을 실행하고 성공 여부를 돌려준다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Interaction")
	bool Interact(const FPDInteractionContextStruct& Context);
};
