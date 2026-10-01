#include "PADO/AbilitySystem/Targeting/PDItemSocketTrailTargeting.h"

#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/AbilitySystem/Targeting/PDTargetingCollision.h"
#include "PADO/Item/PDWorldItemActor.h"

namespace PDItemSocketTrailTargeting
{
	bool IsOwnedOrAttachedTo(
		const AActor& Candidate,
		const AActor& PossibleParent)
	{
		return Candidate.IsOwnedBy(&PossibleParent) ||
			Candidate.IsAttachedTo(&PossibleParent);
	}

#if ENABLE_DRAW_DEBUG
	void DrawTraceSegment(
		const UWorld& World,
		const FVector& Start,
		const FVector& End,
		float Radius,
		float Duration)
	{
		const FVector Axis = End - Start;
		if (Axis.IsNearlyZero())
		{
			::DrawDebugSphere(
				&World, End, Radius, 12, FColor::Silver, false, Duration);
			return;
		}

		::DrawDebugCapsule(
			&World,
			Start + Axis * 0.5f,
			Axis.Size() * 0.5f + Radius,
			Radius,
			FRotationMatrix::MakeFromZ(Axis.GetSafeNormal()).ToQuat(),
			FColor::Silver,
			false,
			Duration);
	}
#endif
}

UPDItemSocketTrailTargeting::UPDItemSocketTrailTargeting()
{
	TargetObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObstructionTraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);
}

bool UPDItemSocketTrailTargeting::Validate(FString& OutError) const
{
	OutError.Reset();
	if (StartSocketName.IsNone() || EndSocketName.IsNone() ||
		StartSocketName == EndSocketName)
	{
		OutError = TEXT("StartSocketName과 EndSocketName은 서로 다른 유효한 이름이어야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(TraceRadius) || TraceRadius <= 0.0f)
	{
		OutError = TEXT("TraceRadius는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (SamplesAlongItem < 2 || SamplesAlongItem > 16)
	{
		OutError = TEXT("SamplesAlongItem은 2 이상 16 이하여야 합니다.");
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

bool UPDItemSocketTrailTargeting::ResolveTraceSource(
	const FPDActionTargetingContext& Context,
	UObject*& OutTraceSource,
	FString* OutError) const
{
	OutTraceSource = nullptr;
	const UPDAbilitySourceComponent* SourceComponent =
		Cast<UPDAbilitySourceComponent>(Context.SourceObject);
	const APDWorldItemActor* ItemActor = SourceComponent
		? Cast<APDWorldItemActor>(SourceComponent->GetOwner())
		: nullptr;
	if (!ItemActor || ItemActor->GetHolder() != Context.SourceActor)
	{
		if (OutError)
		{
			*OutError = TEXT("현재 AbilitySpec의 SourceObject가 Avatar가 들고 있는 PDWorldItemActor의 AbilitySource가 아닙니다.");
		}
		return false;
	}

	UMeshComponent* ItemMesh = ItemActor->GetItemMesh();
	if (!IsValid(ItemMesh))
	{
		if (OutError)
		{
			*OutError = TEXT("아이템의 활성 MeshComponent가 없습니다.");
		}
		return false;
	}

	if (!ItemMesh->DoesSocketExist(StartSocketName) ||
		!ItemMesh->DoesSocketExist(EndSocketName))
	{
		if (OutError)
		{
			*OutError = FString::Printf(
				TEXT("아이템 Mesh에 Trace 소켓 '%s' 또는 '%s'가 없습니다."),
				*StartSocketName.ToString(),
				*EndSocketName.ToString());
		}
		return false;
	}

	OutTraceSource = ItemMesh;
	if (OutError)
	{
		OutError->Reset();
	}
	return true;
}

bool UPDItemSocketTrailTargeting::GetTraceSegment(
	const UObject& TraceSource,
	FVector& OutStart,
	FVector& OutEnd) const
{
	const UMeshComponent* ItemMesh = Cast<UMeshComponent>(&TraceSource);
	if (!ItemMesh || !ItemMesh->DoesSocketExist(StartSocketName) ||
		!ItemMesh->DoesSocketExist(EndSocketName))
	{
		return false;
	}

	OutStart = ItemMesh->GetSocketLocation(StartSocketName);
	OutEnd = ItemMesh->GetSocketLocation(EndSocketName);
	return !OutStart.ContainsNaN() && !OutEnd.ContainsNaN();
}

void UPDItemSocketTrailTargeting::GatherTraceTargets(
	const FPDActionTargetingContext& Context,
	const FVector& PreviousStart,
	const FVector& PreviousEnd,
	const FVector& CurrentStart,
	const FVector& CurrentEnd,
	TArray<FPDActionTarget>& OutTargets) const
{
	AActor* SourceActor = Context.SourceActor;
	UWorld* World = SourceActor ? SourceActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(PDItemSocketTrailTargeting), false);
	QueryParams.bFindInitialOverlaps = true;
	PDTargetingCollision::AddIgnoredSourceActors(
		QueryParams, *SourceActor, Context.SourceObject);

	const FCollisionObjectQueryParams ObjectQuery(TargetObjectTypes);
	TArray<FHitResult> RawHits;
	for (int32 SampleIndex = 0; SampleIndex < SamplesAlongItem; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) /
			static_cast<float>(SamplesAlongItem - 1);
		const FVector TraceStart = FMath::Lerp(PreviousStart, PreviousEnd, Alpha);
		const FVector TraceEnd = FMath::Lerp(CurrentStart, CurrentEnd, Alpha);

		TArray<FHitResult> SampleHits;
		World->SweepMultiByObjectType(
			SampleHits,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			ObjectQuery,
			FCollisionShape::MakeSphere(TraceRadius),
			QueryParams);
		RawHits.Append(MoveTemp(SampleHits));

#if ENABLE_DRAW_DEBUG
		if (bDrawDebugTrace)
		{
			PDItemSocketTrailTargeting::DrawTraceSegment(
				*World,
				TraceStart,
				TraceEnd,
				TraceRadius,
				DebugDrawDuration);
		}
#endif
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	RawHits.Sort(
		[&SourceLocation](const FHitResult& Left, const FHitResult& Right)
		{
			const float LeftDistance = FVector::DistSquared(
				SourceLocation, Left.ImpactPoint);
			const float RightDistance = FVector::DistSquared(
				SourceLocation, Right.ImpactPoint);
			if (!FMath::IsNearlyEqual(LeftDistance, RightDistance))
			{
				return LeftDistance < RightDistance;
			}

			const AActor* LeftActor = Left.GetActor();
			const AActor* RightActor = Right.GetActor();
			return (LeftActor ? LeftActor->GetUniqueID() : MAX_uint32) <
				(RightActor ? RightActor->GetUniqueID() : MAX_uint32);
		});

	const FVector TraceOrigin = (CurrentStart + CurrentEnd) * 0.5f;
	TSet<AActor*> SeenTargets;
	for (const FHitResult& HitResult : RawHits)
	{
		if (OutTargets.Num() >= MaxTargets)
		{
			break;
		}

		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || TargetActor == SourceActor ||
			SeenTargets.Contains(TargetActor) ||
			PDItemSocketTrailTargeting::IsOwnedOrAttachedTo(
				*TargetActor, *SourceActor) ||
			!IsPathUnobstructed(Context, TraceOrigin, *TargetActor))
		{
			continue;
		}

		SeenTargets.Add(TargetActor);
		FPDActionTarget& Target = OutTargets.AddDefaulted_GetRef();
		Target.Actor = TargetActor;
		Target.HitResult = HitResult;
		Target.bHasHitResult = true;

#if ENABLE_DRAW_DEBUG
		if (bDrawDebugTrace)
		{
			::DrawDebugSphere(
				World,
				HitResult.ImpactPoint,
				10.0f,
				12,
				FColor::Red,
				false,
				DebugDrawDuration);
		}
#endif
	}
}

bool UPDItemSocketTrailTargeting::IsPathUnobstructed(
	const FPDActionTargetingContext& Context,
	const FVector& TraceOrigin,
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

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(PDItemSocketTrailObstruction), false);
	PDTargetingCollision::AddIgnoredSourceActors(
		QueryParams, *SourceActor, Context.SourceObject);

	FHitResult ObstructionHit;
	const ECollisionChannel Channel = UEngineTypes::ConvertToCollisionChannel(
		ObstructionTraceChannel.GetValue());
	if (!World->LineTraceSingleByChannel(
		ObstructionHit,
		TraceOrigin,
		TargetActor.GetActorLocation(),
		Channel,
		QueryParams))
	{
		return true;
	}

	const AActor* ObstructionActor = ObstructionHit.GetActor();
	return ObstructionActor == &TargetActor ||
		(ObstructionActor &&
			PDItemSocketTrailTargeting::IsOwnedOrAttachedTo(
				*ObstructionActor, TargetActor));
}
