#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDAimStateProvider.generated.h"

/**
 * 조준 단계다. 카메라 배치, 이동 속도 자세 배율, Targeting 시작점이 이 값을 따른다.
 * 인터페이스의 통화 타입이라 같은 헤더에 둔다.
 */
UENUM(BlueprintType)
enum class EPDAimState : uint8
{
	/** 평상시 3인칭 시점이다. */
	Idle,
	/** 견착이다. 조준 입력을 누르고 있는 동안 유지한다. */
	Shouldered,
	/** 조준이다. 조준 입력을 짧게 눌러 켜고 끈다. */
	Aiming
};

UINTERFACE()
class PADO_API UPDAimStateProvider : public UInterface
{
	GENERATED_BODY()
};

/** 조준 단계를 쓰는 쪽이 구체 캐릭터 타입을 몰라도 되게 하는 계약이다. */
class PADO_API IPDAimStateProvider
{
	GENERATED_BODY()

public:
	virtual EPDAimState GetAimState() const = 0;
};
