#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PDItemTrait.generated.h"

/**
 * 아이템이 선택적으로 갖는 불변 설정 묶음이다.
 *
 * 필요한 Trait만 `UPDItemDefinition::Traits`에 조립한다. 넣지 않은 기능은
 * 디테일 패널에도 나오지 않으므로, 기능과 무관한 아이템이 값을 떠안지 않는다.
 * 새 기능을 추가할 때 `UPDItemDefinition`에 필드를 늘리지 않는다.
 *
 * **런타임 상태를 두지 않는다.** Trait은 Data Asset에 붙어 있어서 같은
 * Definition을 쓰는 모든 아이템이 같은 인스턴스를 공유한다. 여기에 현재 탄약
 * 같은 값을 넣으면 총 두 자루가 상태를 공유하는 버그가 된다. 변하는 값은
 * `APDWorldItemActor`의 Component가 소유한다.
 *
 * 실행 시점에 한 번 동작하는 것은 Trait이 아니라 `UPDActionFragment`다.
 * Trait은 "이 아이템이 무엇인가", Fragment는 "지금 무엇을 하는가"를 다룬다.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PADO_API UPDItemTrait : public UObject
{
	GENERATED_BODY()

public:
	/** 잘못 설정하면 기능이 동작하지 않는 항목만 막는다. */
	virtual bool Validate(FString& OutError) const;
};
