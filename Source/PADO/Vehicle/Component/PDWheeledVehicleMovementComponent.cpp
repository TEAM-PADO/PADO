#include "PADO/Vehicle/Component/PDWheeledVehicleMovementComponent.h"

#include <atomic>

namespace PDWheeledVehicleMovement
{
	/** 물리 스레드에서 도는 차량 시뮬레이션이다. 운전자가 없으면 입력을 중립으로 둔다. */
	class FPDWheeledVehicleSimulation final : public UChaosWheeledVehicleSimulation
	{
	public:
		virtual void TickVehicle(
			UWorld* WorldIn,
			float DeltaTime,
			const FChaosVehicleAsyncInput& InputData,
			FChaosVehicleAsyncOutput& OutputData,
			Chaos::FRigidBodyHandle_Internal* Handle) override
		{
			// 이 시뮬레이션의 입력은 다음 물리 틱에 그 틱의 입력으로 옮겨지고, 서버가
			// 입력을 기록할 때도 여기서 읽는다. 운전자가 떠난 첫 틱 한 번은 이전 입력이 쓰인다.
			if (bDriverless.load(std::memory_order_relaxed))
			{
				FControlInputs NeutralInputs;
				NeutralInputs.TransmissionType = VehicleInputs.TransmissionType;
				NeutralInputs.ParkingEnabled = VehicleInputs.ParkingEnabled;
				VehicleInputs = NeutralInputs;
			}

			UChaosWheeledVehicleSimulation::TickVehicle(
				WorldIn,
				DeltaTime,
				InputData,
				OutputData,
				Handle);
		}

		/** 게임 스레드가 쓰고 물리 스레드가 읽는다. */
		std::atomic<bool> bDriverless = false;
	};
}

UPDWheeledVehicleMovementComponent::UPDWheeledVehicleMovementComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 래그돌은 머신마다 따로 계산하는 연출이라 데디케이티드 서버에는 없다. 바퀴가 래그돌을
	// 밟으면 그 머신의 차만 튀어 서버와 어긋난다. 떨어진 아이템·물리 소품도 같은 타입이라
	// 함께 무시하고, 차체 충돌로만 부딪힌다. 차가 달릴 바닥은 물리 시뮬레이션을 켜지 않는다.
	WheelTraceCollisionResponses.PhysicsBody = ECR_Ignore;

	// 사람(캡슐, 메시)도 밟고 올라가지 않는다. 운전자 머신의 차는 예측으로 서버보다 앞서
	// 사망이 도착하기 전의 피해자 위를 지나가는데, 바퀴가 그 캡슐을 밟으면 그 머신의 차만
	// 크게 튄다(2026-10-02 MCP 측정). 사람은 들이받기 판정과 넉백으로만 다룬다.
	WheelTraceCollisionResponses.Pawn = ECR_Ignore;
}

void UPDWheeledVehicleMovementComponent::SetDriverless(bool bInDriverless)
{
	bDriverless = bInDriverless;

	// 시뮬레이션은 CreatePhysicsVehicle이 언제나 이 프로젝트의 것으로 만든다.
	if (VehicleSimulationPT)
	{
		static_cast<PDWheeledVehicleMovement::FPDWheeledVehicleSimulation*>(
			VehicleSimulationPT.Get())->bDriverless.store(bInDriverless, std::memory_order_relaxed);
	}
}

TUniquePtr<Chaos::FSimpleWheeledVehicle> UPDWheeledVehicleMovementComponent::CreatePhysicsVehicle()
{
	TUniquePtr<PDWheeledVehicleMovement::FPDWheeledVehicleSimulation> Simulation =
		MakeUnique<PDWheeledVehicleMovement::FPDWheeledVehicleSimulation>();
	Simulation->bDriverless.store(bDriverless, std::memory_order_relaxed);
	VehicleSimulationPT = MoveTemp(Simulation);

	// 엔진 바퀴형 무브먼트도 시뮬레이션을 만든 뒤 차량 무브먼트 기본 구현을 부른다.
	return UChaosVehicleMovementComponent::CreatePhysicsVehicle();
}
