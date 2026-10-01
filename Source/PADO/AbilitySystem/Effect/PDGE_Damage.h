#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PDGE_Damage.generated.h"

/**
 * 공용 피해 GE다. 즉시 적용되고, 피해량을 SetByCaller `Data.Damage`로 받는다.
 *
 * 체력을 직접 깎지 않고 메타 속성 Damage에 더한다. 체력으로 옮기는 것과 0 아래로
 * 내려가지 않게 자르는 것은 UPDHealthAttributeSet이 서버에서 한다. 아군과 자기
 * 자신도 거르지 않는다. 막을 대상은 판정이나 대상의 상태가 정한다.
 */
UCLASS(meta = (DisplayName = "PDGE_Damage"))
class PADO_API UPDGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPDGE_Damage();
};
