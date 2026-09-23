#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PDGE_MoveSpeedMultiplier.generated.h"

/**
 * 이동 속도에 배율을 거는 공용 슬로우·헤이스트 GE다.
 *
 * 배율은 SetByCaller `Data.MoveSpeed.Multiplier`로 받는다.
 * Multiply (Compound) 연산이라 여러 개가 겹치면 서로 곱해진다.
 * 0.5 두 개가 걸리면 0.25가 되지 감소량이 더해져 0이 되지 않는다.
 *
 * 지속시간은 Spec에서 정한다. 기본은 Infinite이므로 회수할 때까지 유지한다.
 */
UCLASS(meta = (DisplayName = "PDGE_MoveSpeedMultiplier"))
class PADO_API UPDGE_MoveSpeedMultiplier : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPDGE_MoveSpeedMultiplier();
};
