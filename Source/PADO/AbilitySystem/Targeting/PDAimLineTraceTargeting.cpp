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

DEFINE_LOG_CATEGORY_STATIC(LogPDTargeting, Log, All);

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
	QueryParams.AddIgnoredActor(Context.SourceActor);
	if (const UActorComponent* SourceComponent =
		Cast<UActorComponent>(Context.SourceObject))
	{
		QueryParams.AddIgnoredActor(SourceComponent->GetOwner());
	}

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

	OutAimRotation = SourceActor->GetActorRotation();
	if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
	{
		OutAimRotation = SourcePawn->GetBaseAimRotation();

		// 3인칭 카메라는 캐릭터 뒤 위쪽에 있다. Controller 시점을 써야
		// 화면 중앙이 가리키는 지점과 판정이 일치한다.
		if (const AController* SourceController = SourcePawn->GetController())
		{
			SourceController->GetPlayerViewPoint(OutViewStart, OutAimRotation);
		}
		else
		{
			SourceActor->GetActorEyesViewPoint(OutViewStart, OutAimRotation);
		}
	}
	else
	{
		SourceActor->GetActorEyesViewPoint(OutViewStart, OutAimRotation);
	}

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
