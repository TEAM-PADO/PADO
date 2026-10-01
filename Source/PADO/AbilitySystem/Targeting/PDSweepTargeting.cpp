#include "PADO/AbilitySystem/Targeting/PDSweepTargeting.h"

#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Targeting/PDTargetingCollision.h"

namespace PDSweepTargeting
{
	bool IsOwnedOrAttachedTo(
		const AActor& Candidate,
		const AActor& PossibleParent)
	{
		return Candidate.IsOwnedBy(&PossibleParent) ||
			Candidate.IsAttachedTo(&PossibleParent);
	}
}

UPDSweepTargeting::UPDSweepTargeting()
{
	TargetObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObstructionTraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);
}

bool UPDSweepTargeting::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(SweepDistance) || SweepDistance <= 0.0f)
	{
		OutError = TEXT("SweepDistance는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (Shape == EPDSweepShape::Sphere &&
		(!FMath::IsFinite(SweepRadius) || SweepRadius <= 0.0f))
	{
		OutError = TEXT("SweepRadius는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (Shape == EPDSweepShape::Box &&
		(BoxHalfExtent.ContainsNaN() ||
			BoxHalfExtent.X <= 0.0f ||
			BoxHalfExtent.Y <= 0.0f ||
			BoxHalfExtent.Z <= 0.0f))
	{
		OutError = TEXT("BoxHalfExtent의 각 축은 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(StartForwardOffset) || StartForwardOffset < 0.0f)
	{
		OutError = TEXT("StartForwardOffset은 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(HeightOffset))
	{
		OutError = TEXT("HeightOffset은 유한한 수여야 합니다.");
		return false;
	}

	if (MaxTargets < 1 || MaxTargets > 32)
	{
		OutError = TEXT("MaxTargets는 1 이상 32 이하여야 합니다.");
		return false;
	}

	if (TargetObjectTypes.IsEmpty())
	{
		OutError = TEXT("TargetObjectTypes가 비어 있습니다.");
		return false;
	}

	TSet<ECollisionChannel> SeenChannels;
	for (const TEnumAsByte<EObjectTypeQuery> ObjectType : TargetObjectTypes)
	{
		const ECollisionChannel Channel =
			UEngineTypes::ConvertToCollisionChannel(ObjectType.GetValue());
		if (!FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			OutError = TEXT("TargetObjectTypes에 유효하지 않은 Object Type이 있습니다.");
			return false;
		}

		if (SeenChannels.Contains(Channel))
		{
			OutError = TEXT("TargetObjectTypes에 같은 Object Type이 중복됩니다.");
			return false;
		}
		SeenChannels.Add(Channel);
	}

	if (bRequireUnobstructedPath &&
		UEngineTypes::ConvertToCollisionChannel(
			ObstructionTraceChannel.GetValue()) >= ECC_MAX)
	{
		OutError = TEXT("ObstructionTraceChannel이 유효하지 않습니다.");
		return false;
	}

	return true;
}

void UPDSweepTargeting::GatherTargets(
	const FPDActionTargetingContext& Context,
	FPDActionTargetingResult& OutResult) const
{
	AActor* SourceActor = Context.SourceActor;
	UWorld* World = SourceActor ? SourceActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	FVector Forward = SourceActor->GetActorForwardVector();
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		return;
	}

	const FVector Start = SourceActor->GetActorLocation() +
		Forward * StartForwardOffset +
		FVector::UpVector * HeightOffset;
	const FVector End = Start + Forward * SweepDistance;
	const FQuat ShapeRotation = Shape == EPDSweepShape::Box
		? FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat() *
			BoxRotationOffset.Quaternion()
		: FQuat::Identity;
	const FCollisionShape CollisionShape = Shape == EPDSweepShape::Box
		? FCollisionShape::MakeBox(BoxHalfExtent)
		: FCollisionShape::MakeSphere(SweepRadius);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PDSweepTargeting), false);
	PDTargetingCollision::AddIgnoredSourceActors(
		QueryParams,
		*SourceActor,
		Context.SourceObject);

	TArray<FHitResult> RawHits;
	const FCollisionObjectQueryParams ObjectQuery(TargetObjectTypes);
	World->SweepMultiByObjectType(
		RawHits,
		Start,
		End,
		ShapeRotation,
		ObjectQuery,
		CollisionShape,
		QueryParams);

	const FVector SourceLocation = SourceActor->GetActorLocation();
	RawHits.Sort(
		[&SourceLocation](const FHitResult& Left, const FHitResult& Right)
		{
			const float LeftDistance = FMath::IsFinite(Left.Distance)
				? Left.Distance
				: FVector::Dist(SourceLocation, Left.ImpactPoint);
			const float RightDistance = FMath::IsFinite(Right.Distance)
				? Right.Distance
				: FVector::Dist(SourceLocation, Right.ImpactPoint);
			if (!FMath::IsNearlyEqual(LeftDistance, RightDistance))
			{
				return LeftDistance < RightDistance;
			}

			const AActor* LeftActor = Left.GetActor();
			const AActor* RightActor = Right.GetActor();
			const uint32 LeftId = LeftActor ? LeftActor->GetUniqueID() : MAX_uint32;
			const uint32 RightId = RightActor ? RightActor->GetUniqueID() : MAX_uint32;
			return LeftId < RightId;
		});

	TSet<AActor*> SeenTargets;
	for (const FHitResult& HitResult : RawHits)
	{
		if (OutResult.Targets.Num() >= MaxTargets)
		{
			break;
		}

		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || TargetActor == SourceActor ||
			SeenTargets.Contains(TargetActor) ||
			PDSweepTargeting::IsOwnedOrAttachedTo(*TargetActor, *SourceActor) ||
			!IsPathUnobstructed(Context, *TargetActor))
		{
			continue;
		}

		SeenTargets.Add(TargetActor);
		FPDActionTarget& Target = OutResult.Targets.AddDefaulted_GetRef();
		Target.Actor = TargetActor;
		Target.HitResult = HitResult;
		Target.bHasHitResult = true;
	}

	if (bDrawDebugSweep)
	{
		DrawDebugSweep(*World, Start, End, ShapeRotation, OutResult.Targets);
	}
}

void UPDSweepTargeting::DrawDebugSweep(
	const UWorld& World,
	const FVector& Start,
	const FVector& End,
	const FQuat& ShapeRotation,
	const TArray<FPDActionTarget>& Targets) const
{
#if ENABLE_DRAW_DEBUG
	const FVector Axis = End - Start;
	const FColor ShapeColor = Targets.IsEmpty() ? FColor::Silver : FColor::Green;
	if (Shape == EPDSweepShape::Sphere)
	{
		const float AxisLength = Axis.Size();
		// 구를 쓸어간 형태는 캡슐과 같다. 반지름만큼 양 끝이 늘어난다.
		::DrawDebugCapsule(
			&World,
			Start + Axis * 0.5f,
			AxisLength * 0.5f + SweepRadius,
			SweepRadius,
			FRotationMatrix::MakeFromZ(Axis.GetSafeNormal()).ToQuat(),
			ShapeColor,
			false,
			DebugDrawDuration);
	}
	else
	{
		::DrawDebugBox(
			&World,
			Start,
			BoxHalfExtent,
			ShapeRotation,
			ShapeColor,
			false,
			DebugDrawDuration);
		::DrawDebugBox(
			&World,
			End,
			BoxHalfExtent,
			ShapeRotation,
			ShapeColor,
			false,
			DebugDrawDuration);
		::DrawDebugLine(
			&World,
			Start,
			End,
			ShapeColor,
			false,
			DebugDrawDuration);
	}

	for (const FPDActionTarget& Target : Targets)
	{
		if (Target.bHasHitResult)
		{
			::DrawDebugSphere(
				&World,
				Target.HitResult.ImpactPoint,
				12.0f,
				12,
				FColor::Red,
				false,
				DebugDrawDuration);
		}
	}
#endif
}

bool UPDSweepTargeting::IsPathUnobstructed(
	const FPDActionTargetingContext& Context,
	const AActor& TargetActor) const
{
	if (!bRequireUnobstructedPath)
	{
		return true;
	}

	const AActor* SourceActor = Context.SourceActor;
	UWorld* World = SourceActor ? SourceActor->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	const FVector Start = SourceActor->GetActorLocation() +
		FVector::UpVector * HeightOffset;
	const FVector End = TargetActor.GetActorLocation();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PDSweepObstruction), false);
	PDTargetingCollision::AddIgnoredSourceActors(
		QueryParams,
		*SourceActor,
		Context.SourceObject);

	FHitResult ObstructionHit;
	const ECollisionChannel Channel =
		UEngineTypes::ConvertToCollisionChannel(
			ObstructionTraceChannel.GetValue());
	if (!World->LineTraceSingleByChannel(
		ObstructionHit, Start, End, Channel, QueryParams))
	{
		return true;
	}

	const AActor* ObstructionActor = ObstructionHit.GetActor();
	return ObstructionActor == &TargetActor ||
		(ObstructionActor &&
			PDSweepTargeting::IsOwnedOrAttachedTo(*ObstructionActor, TargetActor));
}
