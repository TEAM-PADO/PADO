#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "PDWheeledVehicleMovementComponent.generated.h"

/**
 * 운전자가 없을 때 중립 입력으로 굴러가는 바퀴형 차량 무브먼트다.
 *
 * Physics Prediction에서 엔진 차량의 물리 시뮬레이션은 그 머신의 로컬
 * PlayerController가 조종할 때만 게임 스레드 입력을 받고, 아니면 마지막으로 적용된
 * 입력을 계속 쓴다(ChaosVehicleManagerAsyncCallback.cpp의 PlayerController 확인).
 * 데디케이티드 서버에는 로컬 PlayerController가 없어서 운전자가 내리거나 운전석을
 * 떠나면 그 사람의 마지막 입력이 남아 차가 계속 달린다. 운전자가 없으면 물리
 * 시뮬레이션이 입력을 직접 비운다.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDWheeledVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	/**
	 * 서버에서 운전자가 있는지 알린다. 운전자가 없으면 스로틀·브레이크·조향이 0이 되어
	 * 관성으로 굴러가다 선다. 서버가 기록한 이 입력은 엔진이 다른 머신에 복제한다.
	 */
	void SetDriverless(bool bInDriverless);

	bool IsDriverless() const { return bDriverless; }

protected:
	/** 엔진 바퀴형 무브먼트와 같고, 물리 시뮬레이션만 이 프로젝트의 것으로 만든다. */
	virtual TUniquePtr<Chaos::FSimpleWheeledVehicle> CreatePhysicsVehicle() override;

private:
	/** 물리 상태를 다시 만들어도 새 시뮬레이션이 이어받는다. */
	bool bDriverless = false;
};
