// Fill out your copyright notice in the Description page of Project Settings.


#include "PDZombieMoveToTargetProcessor.h"

#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Example/MassSimpleMovementTrait.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "PDZombieMoveStateFragment.h"

UPDZombieMoveToTargetProcessor::UPDZombieMoveToTargetProcessor()
    : EntityQuery(*this)
{
    bAutoRegisterWithProcessingPhases = true;
    bRequiresGameThreadExecution = true;
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);

    // Simple Movement Processor가 Velocity를 읽기 전에 실행합니다.
    ExecutionOrder.ExecuteBefore.Add(
        UE::Mass::ProcessorGroupNames::Avoidance);
}

void UPDZombieMoveToTargetProcessor::ConfigureQueries(
    const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.AddRequirement<FTransformFragment>(
        EMassFragmentAccess::ReadOnly);

    EntityQuery.AddRequirement<FMassVelocityFragment>(
        EMassFragmentAccess::ReadWrite);

    // 현재 Config의 Simple Movement Trait을 가진 Entity만 대상으로 합니다.
    EntityQuery.AddTagRequirement<FMassSimpleMovementTag>(
        EMassFragmentPresence::All);
    
    EntityQuery.AddRequirement<FPDZombieMoveStateFragment>(
    EMassFragmentAccess::ReadOnly);
}

void UPDZombieMoveToTargetProcessor::Execute(
    FMassEntityManager& EntityManager,
    FMassExecutionContext& Context)
{
    UWorld* World = Context.GetWorld();
    if (!World)
    {
        return;
    }

    if (!TargetPoint.IsValid())
    {
        for (TActorIterator<ATargetPoint> It(World); It; ++It)
        {
            if (It->ActorHasTag(TEXT("PDZombieTarget")))
            {
                TargetPoint = *It;
                break;
            }
        }
    }

    if (!TargetPoint.IsValid())
    {
        return;
    }

    const FVector TargetLocation = TargetPoint->GetActorLocation();

    EntityQuery.ForEachEntityChunk(Context, [TargetLocation](FMassExecutionContext& ChunkContext)
        {
            constexpr float MoveSpeed = 200.0f; // 초속
            constexpr float AcceptanceRadius = 100.0f; // cm

            const TConstArrayView<FTransformFragment> Transforms =
                ChunkContext.GetFragmentView<FTransformFragment>(); // transform 얻기

            const TArrayView<FMassVelocityFragment> Velocities =
                ChunkContext.GetMutableFragmentView<FMassVelocityFragment>(); // velocity 얻기
        
            const TConstArrayView<FPDZombieMoveStateFragment>
                    MoveStates = ChunkContext.GetFragmentView<FPDZombieMoveStateFragment>(); // 상태 배열 읽기

            for (FMassExecutionContext::FEntityIterator
                It = ChunkContext.CreateEntityIterator(); It; ++It)
            {
                if (MoveStates[It].State == EPDZombieMoveState::Idle)
                {
                    Velocities[It].Value = FVector::ZeroVector;
                    continue;
                }
                
                FVector ToTarget = TargetLocation - Transforms[It].GetTransform().GetLocation();

                ToTarget.Z = 0.0f;

                if (ToTarget.SizeSquared() <=  FMath::Square(AcceptanceRadius))
                {
                    Velocities[It].Value = FVector::ZeroVector;
                }
                else
                {
                    Velocities[It].Value = ToTarget.GetSafeNormal() * MoveSpeed;
                }
            }
        });
}
