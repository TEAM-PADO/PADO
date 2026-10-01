#pragma once

#include "CoreMinimal.h"
#include "PDLifeState.generated.h"

/** 몸의 생명 상태다. 체력이 0이 되면 빈사를 거쳐(설정에 따라 건너뛰고) 사망한다. */
UENUM(BlueprintType)
enum class EPDLifeState : uint8
{
	Alive,

	/** 빈사다. 체력이 0이고 손을 쓸 수 없다. 시간이 지나거나 다시 피해를 받으면 사망하고, 살리면 돌아온다. */
	Downed,

	/** 사망이다. 되돌아오지 않는다. 다시 살아나는 것은 새 몸이다. */
	Dead
};
