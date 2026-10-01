#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PDControllableVehicle.generated.h"

class AController;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPDControllableVehicle : public UInterface
{
	GENERATED_BODY()
};

/**
 * 조종석이 채워지고 비워질 때 탈것이 조종 권한을 넘기고 거두는 계약이다.
 *
 * 좌석 구성은 탈것의 이동 방식을 모른다. 권한을 어떻게 넘길지는 탈것이 정한다.
 * 바퀴형은 빙의하지 않고 Owner와 OverrideController를 넘기며, 보행형처럼 엔진
 * 이동이 빙의를 전제하는 탈것은 같은 함수에서 빙의로 넘긴다.
 */
class PADO_API IPDControllableVehicle
{
	GENERATED_BODY()

public:
	/** 서버에서만 부른다. null이면 조종 권한을 거둔다. */
	virtual void SetVehicleController(AController* NewController) = 0;

	virtual AController* GetVehicleController() const = 0;
};
