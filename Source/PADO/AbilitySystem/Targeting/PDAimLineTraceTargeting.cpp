#include "PADO/AbilitySystem/Targeting/PDAimLineTraceTargeting.h"

#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/AbilitySystem/Targeting/PDTargetingCollision.h"
#include "PADO/Core/PDViewPoint.h"
#include "PADO/Item/PDWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDTargeting, Log, All);

namespace PDAimLineTraceTargeting
{
	/** 가림 판정이 끝점 너머로 더 보는 거리다. 표면을 넘을 만큼만 준다. */
	constexpr float SurfaceProbeDistance = 10.0f;
}

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
	FPDActionTargetingResult& OutResult) const
{
	if (!Context.SourceActor)
	{
		return;
	}

	UWorld* World = Context.SourceActor->GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PDAimLineTrace), true);
	// 탄착 연출이 표면에 따라 달라질 수 있도록 멈춘 곳의 재질을 받아 둔다.
	QueryParams.bReturnPhysicalMaterial = true;
	PDTargetingCollision::AddIgnoredSourceActors(
		QueryParams, *Context.SourceActor, Context.SourceObject);

	FVector Start;
	FVector End;
	if (!ResolveTrace(Context, *World, QueryParams, Start, End))
	{
		return;
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

	// 끝점은 1단계 판정이 맞힌 표면 위에 있다. 딱 거기서 끝내면 부동소수 오차로 그
	// 표면을 놓친다. 벽에 쏜 탄이 허공에 멈춘 것으로 나오고, 차량처럼 Visibility를
	// 막는 대상은 대상에서 빠진다. 가림과 대상 모두 표면 너머까지 조금 더 본다.
	const FVector Direction = (End - Start).GetSafeNormal();
	const FVector ProbeEnd = End + Direction * PDAimLineTraceTargeting::SurfaceProbeDistance;
	FHitResult ObstructionHit;
	bool bObstructed = false;
	float ObstructionDistanceSquared = TNumericLimits<float>::Max();
	if (bRequireUnobstructedPath)
	{
		const ECollisionChannel Channel = UEngineTypes::ConvertToCollisionChannel(
			ObstructionTraceChannel.GetValue());
		bObstructed = Channel < ECC_MAX && World->LineTraceSingleByChannel(
			ObstructionHit,
			Start,
			ProbeEnd,
			Channel,
			QueryParams);
		if (bObstructed)
		{
			ObstructionDistanceSquared = FVector::DistSquared(
				Start, ObstructionHit.ImpactPoint);
		}
	}

	TArray<FHitResult> Hits;
	World->LineTraceMultiByObjectType(Hits, Start, ProbeEnd, ObjectParams, QueryParams);
	TArray<FPDActionTarget>& OutTargets = OutResult.Targets;
	TSet<TObjectPtr<AActor>> SeenActors;
	for (const FHitResult& Hit : Hits)
	{
		// 탄을 막은 것이 이 대상 자신이면(차량) 거리를 비교하지 않는다. 같은 표면을 두
		// 판정이 따로 재서 오차만큼 뒤에 나올 수 있다.
		AActor* HitActor = Hit.GetActor();
		const bool bBlockedByThisTarget = bObstructed && ObstructionHit.GetActor() == HitActor;
		if (!IsValid(HitActor) || SeenActors.Contains(HitActor) ||
			(TargetActorClass && !HitActor->IsA(TargetActorClass)) ||
			(!bBlockedByThisTarget &&
				FVector::DistSquared(Start, Hit.ImpactPoint) >
					ObstructionDistanceSquared + UE_KINDA_SMALL_NUMBER))
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

	// 탄이 멈춘 곳이다. 대상 수를 다 채웠으면 마지막 대상에서, 가려졌으면
	// 가린 곳에서 멈춘다. 둘 다 아니면 사거리 끝까지 날아간 것이다.
	// 가림을 보지 않는 설정이면 대상 판정처럼 탄도 벽을 지나간다.
	FHitResult& Shot = OutResult.ShotResult;
	if (OutTargets.Num() >= MaxTargets)
	{
		Shot = OutTargets.Last().HitResult;
		Shot.bBlockingHit = true;
	}
	else if (bObstructed)
	{
		Shot = ObstructionHit;
	}
	else
	{
		Shot = FHitResult(Start, End);
		Shot.Location = End;
		Shot.ImpactPoint = End;
		Shot.Distance = FVector::Dist(Start, End);
		// 허공에서 멈춘 탄은 표면이 없다. 쏜 쪽을 보는 면에 멈춘 것처럼 둔다.
		Shot.Normal = -Direction;
		Shot.ImpactNormal = -Direction;
	}
	Shot.TraceStart = Start;
	Shot.TraceEnd = End;
	OutResult.bHasShotResult = true;

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
		if (Shot.bBlockingHit)
		{
			DrawDebugPoint(
				World,
				Shot.ImpactPoint,
				8.0f,
				FColor::Yellow,
				false,
				DebugDrawDuration);
		}
	}
#endif
}

EPDAimTraceOrigin UPDAimLineTraceTargeting::ResolveOriginForAimState(
	const FPDActionTargetingContext& Context) const
{
	const IPDAimStateProvider* AimProvider =
		Cast<IPDAimStateProvider>(Context.SourceActor);
	if (!AimProvider)
	{
		return TraceOrigin;
	}

	const EPDAimTraceOrigin* Override =
		OriginByAimState.Find(AimProvider->GetAimState());
	return Override ? *Override : TraceOrigin;
}

bool UPDAimLineTraceTargeting::ResolveAimPoint(
	const FPDActionTargetingContext& Context,
	const UWorld& World,
	const FCollisionQueryParams& QueryParams,
	FVector& OutViewStart,
	FRotator& OutAimRotation,
	FVector& OutAimPoint) const
{
	AActor* SourceActor = Context.SourceActor;
	if (!SourceActor)
	{
		return false;
	}

	// 3인칭 카메라는 캐릭터 뒤 위쪽에 있다. Controller 시점을 써야
	// 화면 중앙이 가리키는 지점과 판정이 일치한다.
	PDViewPoint::GetActorViewPoint(*SourceActor, OutViewStart, OutAimRotation);

	const FVector ViewEnd =
		OutViewStart + OutAimRotation.Vector() * TraceDistance;

	// 아무것도 맞지 않으면 사거리 끝을 조준점으로 삼는다.
	FHitResult ViewHit;
	OutAimPoint = World.LineTraceSingleByChannel(
		ViewHit, OutViewStart, ViewEnd, ECC_Visibility, QueryParams)
			? ViewHit.ImpactPoint
			: ViewEnd;
	return !OutViewStart.ContainsNaN() && !OutAimPoint.ContainsNaN();
}

bool UPDAimLineTraceTargeting::ResolveTrace(
	const FPDActionTargetingContext& Context,
	const UWorld& World,
	const FCollisionQueryParams& QueryParams,
	FVector& OutStart,
	FVector& OutEnd) const
{
	AActor* SourceActor = Context.SourceActor;
	FVector ViewStart;
	FRotator AimRotation;
	FVector AimPoint;
	if (!SourceActor ||
		!ResolveAimPoint(Context, World, QueryParams, ViewStart, AimRotation, AimPoint))
	{
		return false;
	}

	// 1단계에서 구한 조준점으로 2단계 판정을 쏜다. 총구에서 나가도
	// 화면 중앙에 맞는다.
	OutStart = ViewStart;
	switch (ResolveOriginForAimState(Context))
	{
	case EPDAimTraceOrigin::SourceViewPoint:
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
			if (ItemMesh && ItemMesh->DoesSocketExist(ItemSocketName))
			{
				OutStart = ItemMesh->GetSocketLocation(ItemSocketName);
				break;
			}

			// 소켓이 없다고 발사를 막지는 않는다. 검증용 메시에는 총구 소켓이
			// 없는 경우가 흔하므로 시점 기준으로 내려서 쏜다.
			UE_LOG(
				LogPDTargeting,
				Warning,
				TEXT("'%s'에 소켓 '%s'가 없어 시점 기준으로 대체합니다."),
				*GetNameSafe(ItemActor),
				*ItemSocketName.ToString());
			break;
		}

	case EPDAimTraceOrigin::SourceActor:
	default:
		OutStart = SourceActor->GetActorLocation();
		break;
	}

	OutStart += AimRotation.RotateVector(OriginOffset);
	OutEnd = AimPoint;
	return !OutStart.ContainsNaN() && !OutEnd.ContainsNaN();
}
