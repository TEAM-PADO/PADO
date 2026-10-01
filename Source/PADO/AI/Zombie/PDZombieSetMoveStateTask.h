#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "PDZombieMoveStateFragment.h"
#include "PDZombieSetMoveStateTask.generated.h"

class UMassSignalSubsystem;

USTRUCT()
struct FPDZombieSetMoveStateTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Zombie")
	EPDZombieMoveState State = EPDZombieMoveState::Idle;

	UPROPERTY(EditAnywhere, Category = "Zombie", meta = (ClampMin = "0.1"))
	float Duration = 2.0f;

	float ElapsedTime = 0.0f;
};

USTRUCT(meta = (DisplayName = "PD Zombie Set Move State"))
struct FPDZombieSetMoveStateTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPDZombieSetMoveStateTaskInstanceData;

protected:
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		float DeltaTime) const override;

	TStateTreeExternalDataHandle<FPDZombieMoveStateFragment> StateHandle;
	TStateTreeExternalDataHandle<UMassSignalSubsystem> SignalHandle;
};