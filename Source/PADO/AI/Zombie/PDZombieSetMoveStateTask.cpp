#include "PDZombieSetMoveStateTask.h"

#include "MassSignalSubsystem.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FPDZombieSetMoveStateTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateHandle);
	Linker.LinkExternalData(SignalHandle);
	return true;
}

EStateTreeRunStatus FPDZombieSetMoveStateTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.0f;

	FPDZombieMoveStateFragment& MoveState =
		Context.GetExternalData(StateHandle);
	MoveState.State = Data.State;

	const FMassStateTreeExecutionContext& MassContext =
		static_cast<const FMassStateTreeExecutionContext&>(Context);

	UMassSignalSubsystem& Signals =
		Context.GetExternalData(SignalHandle);

	// Mass StateTree가 Duration 후 다시 평가되도록 깨웁니다.
	Signals.DelaySignalEntityDeferred(
		MassContext.GetMassEntityExecutionContext(),
		UE::Mass::Signals::DelayedTransitionWakeup,
		MassContext.GetEntity(),
		Data.Duration);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPDZombieSetMoveStateTask::Tick(
	FStateTreeExecutionContext& Context,
	float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime += DeltaTime;

	return Data.ElapsedTime >= Data.Duration
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Running;
}