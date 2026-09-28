#include "PADO/AbilitySystem/Targeting/PDOverlapTargeting.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDOverlapTargeting, Log, All);

UPDOverlapTargeting::UPDOverlapTargeting()
{
	TargetActorClass = APawn::StaticClass();
}

bool UPDOverlapTargeting::Validate(FString& OutError) const
{
	OutError.Reset();
	if (MaxTargets < 1 || MaxTargets > 32)
	{
		OutError = TEXT("MaxTargets는 1 이상 32 이하여야 합니다.");
		return false;
	}

	return true;
}

UPrimitiveComponent* UPDOverlapTargeting::ResolveOverlapComponent(
	AActor& SourceActor) const
{
	if (UPrimitiveComponent* Referenced =
		Cast<UPrimitiveComponent>(OverlapComponent.GetComponent(&SourceActor)))
	{
		return Referenced;
	}

	return Cast<UPrimitiveComponent>(SourceActor.GetRootComponent());
}

void UPDOverlapTargeting::GatherTargets(
	const FPDActionTargetingContext& Context,
	FPDActionTargetingResult& OutResult) const
{
	AActor* SourceActor = Context.SourceActor;
	if (!IsValid(SourceActor))
	{
		return;
	}

	UPrimitiveComponent* Volume = ResolveOverlapComponent(*SourceActor);
	if (!Volume)
	{
		UE_LOG(
			LogPDOverlapTargeting,
			Warning,
			TEXT("소스 '%s'에서 판정에 쓸 Primitive 볼륨을 찾지 못했습니다."),
			*SourceActor->GetName());
		return;
	}

	TArray<AActor*> OverlappingActors;
	Volume->GetOverlappingActors(OverlappingActors, TargetActorClass);

	for (AActor* Candidate : OverlappingActors)
	{
		if (OutResult.Targets.Num() >= MaxTargets)
		{
			break;
		}

		// 소스 자신과 소스에 딸린 Actor는 대상이 아니다.
		if (!IsValid(Candidate) || Candidate == SourceActor ||
			Candidate->IsOwnedBy(SourceActor) ||
			Candidate->IsAttachedTo(SourceActor))
		{
			continue;
		}

		// Actor 단위 Overlap에는 상호작용 Sphere 같은 보조 컴포넌트도
		// 포함된다. 실제 몸체인 Root Primitive가 겹친 경우만 대상으로 삼는다.
		UPrimitiveComponent* CandidateRoot =
			Cast<UPrimitiveComponent>(Candidate->GetRootComponent());
		if (!CandidateRoot || !Volume->IsOverlappingComponent(CandidateRoot))
		{
			continue;
		}

		FPDActionTarget& Target = OutResult.Targets.AddDefaulted_GetRef();
		Target.Actor = Candidate;
	}
}
