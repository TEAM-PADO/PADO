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
