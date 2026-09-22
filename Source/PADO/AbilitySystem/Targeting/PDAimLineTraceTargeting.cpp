#include "PADO/AbilitySystem/Targeting/PDAimLineTraceTargeting.h"

#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

UPDAimLineTraceTargeting::UPDAimLineTraceTargeting()
{
	TargetObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObstructionTraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);
}

bool UPDAimLineTraceTargeting::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(TraceDistance) || TraceDistance <= 0.0f)
	{
		OutError = TEXT("TraceDistance는 0보다 큰 유한한 값이어야 합니다.");
		return false;
	}
	if (TraceOrigin == EPDAimTraceOrigin::ItemSocket && ItemSocketName.IsNone())
	{
		OutError = TEXT("ItemSocket Origin에는 ItemSocketName이 필요합니다.");
		return false;
	}
	if (TargetObjectTypes.IsEmpty())
	{
		OutError = TEXT("TargetObjectTypes가 비어 있습니다.");
		return false;
	}
	if (MaxTargets < 1 || MaxTargets > 32)
	{
		OutError = TEXT("MaxTargets는 1 이상 32 이하여야 합니다.");
		return false;
	}
	return true;
}

void UPDAimLineTraceTargeting::GatherTargets(
	const FPDActionTargetingContext& Context,
	TArray<FPDActionTarget>& OutTargets) const
{
	FVector Start;
	FVector End;
	if (!ResolveTrace(Context, Start, End) || !Context.SourceActor)
	{
		return;
	}

	UWorld* World = Context.SourceActor->GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PDAimLineTrace), true);
	QueryParams.AddIgnoredActor(Context.SourceActor);
	if (const UActorComponent* SourceComponent =
		Cast<UActorComponent>(Context.SourceObject))
	{
		QueryParams.AddIgnoredActor(SourceComponent->GetOwner());
	}

	FCollisionObjectQueryParams ObjectParams;
	for (const TEnumAsByte<EObjectTypeQuery> ObjectType : TargetObjectTypes)
	{
		const ECollisionChannel Channel =
			UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
		if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
	}

	float ObstructionDistanceSquared = TNumericLimits<float>::Max();
	if (bRequireUnobstructedPath)
	{
		FHitResult ObstructionHit;
		const ECollisionChannel Channel = UEngineTypes::ConvertToCollisionChannel(
			ObstructionTraceChannel.GetValue());
		if (Channel < ECC_MAX && World->LineTraceSingleByChannel(
			ObstructionHit, Start, End, Channel, QueryParams))
		{
			ObstructionDistanceSquared = FVector::DistSquared(
				Start, ObstructionHit.ImpactPoint);
		}
	}

	TArray<FHitResult> Hits;
	World->LineTraceMultiByObjectType(Hits, Start, End, ObjectParams, QueryParams);
	TSet<TObjectPtr<AActor>> SeenActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsValid(HitActor) || SeenActors.Contains(HitActor) ||
			(TargetActorClass && !HitActor->IsA(TargetActorClass)) ||
			FVector::DistSquared(Start, Hit.ImpactPoint) >
				ObstructionDistanceSquared + UE_KINDA_SMALL_NUMBER)
		{
			continue;
		}

		SeenActors.Add(HitActor);
		FPDActionTarget& Target = OutTargets.AddDefaulted_GetRef();
		Target.Actor = HitActor;
		Target.HitResult = Hit;
		Target.bHasHitResult = true;
		if (OutTargets.Num() >= MaxTargets)
		{
			break;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTrace)
	{
		DrawDebugLine(
			World,
			Start,
			End,
			OutTargets.IsEmpty() ? FColor::Silver : FColor::Red,
			false,
			DebugDrawDuration,
			0,
			1.0f);
	}
#endif
}

bool UPDAimLineTraceTargeting::ResolveTrace(
	const FPDActionTargetingContext& Context,
	FVector& OutStart,
	FVector& OutEnd) const
{
	AActor* SourceActor = Context.SourceActor;
	if (!SourceActor)
	{
		return false;
	}

	FRotator AimRotation = SourceActor->GetActorRotation();
	if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
	{
		AimRotation = SourcePawn->GetBaseAimRotation();
	}

	switch (TraceOrigin)
	{
	case EPDAimTraceOrigin::SourceViewPoint:
		// 3인칭에서는 카메라가 캐릭터 뒤 위쪽에 있어서, 폰의 눈 위치에서 쏘면
		// 조준 방향이 같아도 화면 중앙보다 일정하게 위로 빗나간다. Controller의
		// 시점을 쓰면 화면 중앙이 곧 탄착점이 된다. AI Controller는
		// GetPlayerViewPoint가 폰의 눈 위치를 돌려주므로 동작이 바뀌지 않는다.
		if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
		{
			if (const AController* SourceController = SourcePawn->GetController())
			{
				SourceController->GetPlayerViewPoint(OutStart, AimRotation);
				break;
			}
		}
		SourceActor->GetActorEyesViewPoint(OutStart, AimRotation);
		break;

	case EPDAimTraceOrigin::ItemSocket:
		{
			const UPDAbilitySourceComponent* SourceComponent =
				Cast<UPDAbilitySourceComponent>(Context.SourceObject);
			const APDWorldItemActor* ItemActor = SourceComponent
				? Cast<APDWorldItemActor>(SourceComponent->GetOwner())
				: nullptr;
			const UMeshComponent* ItemMesh = ItemActor
				? ItemActor->GetItemMesh()
				: nullptr;
			if (!ItemMesh || !ItemMesh->DoesSocketExist(ItemSocketName))
			{
				return false;
			}
			OutStart = ItemMesh->GetSocketLocation(ItemSocketName);
			break;
		}

	case EPDAimTraceOrigin::SourceActor:
	default:
		OutStart = SourceActor->GetActorLocation();
		break;
	}

	OutStart += AimRotation.RotateVector(OriginOffset);
	OutEnd = OutStart + AimRotation.Vector() * TraceDistance;
	return !OutStart.ContainsNaN() && !OutEnd.ContainsNaN();
}
