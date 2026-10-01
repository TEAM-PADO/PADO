#include "PADO/Interaction/Component/PDInteractionComponent.h"

#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Core/PDViewPoint.h"
#include "PADO/Interaction/Interface/PDInteractable.h"

UPDInteractionComponent::UPDInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UPDInteractionComponent::TryInteract()
{
	AActor* Owner = GetOwner();
	AActor* Target = nullptr;
	UPrimitiveComponent* AimedComponent = nullptr;
	if (!Owner || !IsOwnerAbleToInteract() || !FindInteractionTarget(Target, AimedComponent))
	{
		return false;
	}

	if (Owner->HasAuthority())
	{
		return InteractWithTarget(Target, AimedComponent);
	}

	ServerInteract(Target, AimedComponent);
	return true;
}

void UPDInteractionComponent::ServerInteract_Implementation(
	AActor* Target,
	UPrimitiveComponent* AimedComponent)
{
	InteractWithTarget(Target, AimedComponent);
}

bool UPDInteractionComponent::InteractWithTarget(
	AActor* Target,
	UPrimitiveComponent* AimedComponent)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !IsValid(Target) || !IsOwnerAbleToInteract())
	{
		return false;
	}

	// 조준 컴포넌트는 대상 안의 위치를 가리키는 값이다. 다른 액터의 것은 의미가 없다.
	if (AimedComponent && AimedComponent->GetOwner() != Target)
	{
		AimedComponent = nullptr;
	}

	// 먼저 도착한 요청이 대상을 바꿨을 수 있다. 선점된 대상은 여기서 거부된다.
	const FPDInteractionContextStruct Context = MakeContext(AimedComponent);
	if (!CanInteractWith(Target, Context))
	{
		return false;
	}

	return IPDInteractable::Execute_Interact(Target, Context);
}

bool UPDInteractionComponent::FindInteractionTarget(
	AActor*& OutTarget,
	UPrimitiveComponent*& OutAimedComponent) const
{
	OutTarget = nullptr;
	OutAimedComponent = nullptr;

	const UWorld* World = GetWorld();
	const FPDInteractionContextStruct BaseContext = MakeContext(nullptr);
	if (!World || !BaseContext.Instigator || InteractionReach <= 0.0f)
	{
		return false;
	}

	// 시선 Sweep만으로는 바닥에 놓인 대상을 고를 수 없다. 시점이 캐릭터 중심
	// 높이에서 수평으로 나가는 동안 바닥의 아이템은 그보다 한참 아래에 있어서
	// 스쳐 지나간다. 그래서 시선에 걸린 것이 없으면 가장 가까운 후보를 고른다.
	return FindAimedTarget(*World, BaseContext, OutTarget, OutAimedComponent) ||
		FindNearestTarget(*World, BaseContext, OutTarget, OutAimedComponent);
}

float UPDInteractionComponent::GetDistanceToTarget(const AActor* Target) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Target)
	{
		return TNumericLimits<float>::Max();
	}

	float NearestDistance = TNumericLimits<float>::Max();
	bool bMeasured = false;
	const FVector Origin = Owner->GetActorLocation();
	Target->ForEachComponent<UPrimitiveComponent>(
		false,
		[&](const UPrimitiveComponent* Component)
		{
			if (!Component->IsCollisionEnabled())
			{
				return;
			}

			FVector ClosestPoint;
			const float Distance = Component->GetDistanceToCollision(Origin, ClosestPoint);
			if (Distance >= 0.0f)
			{
				NearestDistance = FMath::Min(NearestDistance, Distance);
				bMeasured = true;
			}
		});

	return bMeasured
		? NearestDistance
		: FVector::Dist(Origin, Target->GetActorLocation());
}

FPDInteractionContextStruct UPDInteractionComponent::MakeContext(
	UPrimitiveComponent* AimedComponent) const
{
	FPDInteractionContextStruct Context;
	Context.Instigator = Cast<APDCharacterBase>(GetOwner());
	Context.AimedComponent = AimedComponent;
	return Context;
}

bool UPDInteractionComponent::CanInteractWith(
	const AActor* Target,
	const FPDInteractionContextStruct& Context) const
{
	return IsValid(Target) &&
		Context.Instigator &&
		Target != Context.Instigator.Get() &&
		Target->Implements<UPDInteractable>() &&
		IPDInteractable::Execute_CanInteract(Target, Context);
}

bool UPDInteractionComponent::FindAimedTarget(
	const UWorld& World,
	const FPDInteractionContextStruct& BaseContext,
	AActor*& OutTarget,
	UPrimitiveComponent*& OutAimedComponent) const
{
	AActor* Owner = GetOwner();
	FVector ViewLocation;
	FRotator ViewRotation;
	PDViewPoint::GetActorViewPoint(*Owner, ViewLocation, ViewRotation);
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;

	FCollisionQueryParams QueryParams(TEXT("PDInteractTrace"), false, Owner);
	TArray<FHitResult> Hits;
	World.SweepMultiByChannel(
		Hits,
		ViewLocation,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);

	// Sweep 결과는 시작점에서 가까운 순서다. 카메라가 캐릭터 뒤에 있으므로
	// 먼저 걸리는 것이 곧 시선상 가장 앞의 후보다.
	for (const FHitResult& Hit : Hits)
	{
		AActor* Candidate = Hit.GetActor();
		FPDInteractionContextStruct Context = BaseContext;
		Context.AimedComponent = Hit.GetComponent();
		if (GetDistanceToTarget(Candidate) <= InteractionReach &&
			CanInteractWith(Candidate, Context))
		{
			OutTarget = Candidate;
			OutAimedComponent = Hit.GetComponent();
			return true;
		}
	}

	return false;
}

bool UPDInteractionComponent::FindNearestTarget(
	const UWorld& World,
	const FPDInteractionContextStruct& BaseContext,
	AActor*& OutTarget,
	UPrimitiveComponent*& OutAimedComponent) const
{
	AActor* Owner = GetOwner();
	FCollisionQueryParams QueryParams(TEXT("PDInteractOverlap"), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World.OverlapMultiByChannel(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(InteractionReach),
		QueryParams);

	// 한 대상의 여러 컴포넌트가 걸릴 수 있다. 컴포넌트 단위로 가장 가까운 것을 고른다.
	float NearestDistance = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		UPrimitiveComponent* Component = Overlap.GetComponent();
		if (!Component)
		{
			continue;
		}

		const float Distance = GetDistanceToComponent(*Component);
		if (Distance > InteractionReach || Distance >= NearestDistance)
		{
			continue;
		}

		FPDInteractionContextStruct Context = BaseContext;
		Context.AimedComponent = Component;
		if (CanInteractWith(Candidate, Context))
		{
			NearestDistance = Distance;
			OutTarget = Candidate;
			OutAimedComponent = Component;
		}
	}

	return OutTarget != nullptr;
}

float UPDInteractionComponent::GetDistanceToComponent(
	const UPrimitiveComponent& Component) const
{
	const FVector Origin = GetOwner()->GetActorLocation();
	FVector ClosestPoint;
	const float Distance = Component.GetDistanceToCollision(Origin, ClosestPoint);

	// 충돌체가 없어 잴 수 없으면(-1) 컴포넌트 위치로 잰다.
	return Distance >= 0.0f
		? Distance
		: FVector::Dist(Origin, Component.GetComponentLocation());
}

bool UPDInteractionComponent::IsOwnerAbleToInteract() const
{
	// 빈사·사망한 몸은 줍거나 타지 않는다. 쓰러지기 직전에 보낸 요청이 늦게 도착해도 거부한다.
	const APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	return !Character || Character->IsAlive();
}
