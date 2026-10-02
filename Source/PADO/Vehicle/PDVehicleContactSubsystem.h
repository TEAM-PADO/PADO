#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PDVehicleContactSubsystem.generated.h"

class FPhysScene_Chaos;
class UPrimitiveComponent;

namespace PDVehicleContact
{
	class FContactCallback;
}

/**
 * 탈것과 "탈것에 밀리기만 하는 몸"의 물리 접촉을 한쪽 방향으로 만든다.
 *
 * 래그돌은 머신마다 따로 계산하는 연출이라 데디케이티드 서버에는 없다. 양방향 충돌이면
 * 클라이언트의 래그돌이 그 머신의 차를 밀어 올리고, 운전자 머신에서는 서버와 어긋나
 * 되감기 보정이 생긴다. 그래서 둘이 닿을 때 그 접촉에서만 차를 무한히 무거운 물체로
 * 본다(Chaos 접촉 수정). 차는 래그돌을 실제 충돌로 밀어내고, 래그돌은 차를 움직이지 못한다.
 *
 * 등록한 바디가 물리에서 사라지면(몸이 지워짐) 물리 스레드가 스스로 목록에서 뺀다.
 * 물리 상태를 다시 만든 컴포넌트는 다시 등록해야 한다.
 */
UCLASS()
class PADO_API UPDVehicleContactSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** 탈것의 차체다. 이 바디는 밀리기만 하는 몸에 밀리지 않는다. */
	void RegisterVehicle(UPrimitiveComponent& Body);

	/** 탈것에 밀리기만 하고 탈것을 밀지 못하는 몸이다(래그돌). */
	void RegisterPassiveBody(UPrimitiveComponent& Body);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void Register(UPrimitiveComponent& Body, bool bVehicle);

	/** 이 월드의 물리 솔버에 콜백을 등록한다. 물리 씬이 없으면 null이다. */
	PDVehicleContact::FContactCallback* EnsureCallback();

	/** 물리 씬이 사라지기 전에 콜백을 해제한다. 엔진도 이 시점에 자기 콜백을 해제한다. */
	void HandlePhysSceneTerm(FPhysScene_Chaos* PhysScene);
	void ReleaseCallback();

	PDVehicleContact::FContactCallback* Callback = nullptr;
	FPhysScene_Chaos* CallbackScene = nullptr;
	FDelegateHandle PhysSceneTermHandle;
};
