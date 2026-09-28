#include "PADO/AbilitySystem/Fragment/PDDropHeldItemFragment.h"

#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"

bool UPDDropHeldItemFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(HorizontalImpulse) || HorizontalImpulse < 0.0f)
	{
		OutError = TEXT("HorizontalImpulse는 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(VerticalImpulse) || VerticalImpulse < 0.0f)
	{
		OutError = TEXT("VerticalImpulse는 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (bApplyImpulse && FMath::IsNearlyZero(HorizontalImpulse) &&
		FMath::IsNearlyZero(VerticalImpulse))
	{
		OutError = TEXT("Impulse를 사용할 때 수평·수직 값이 모두 0일 수 없습니다.");
		return false;
	}

	return true;
}

bool UPDDropHeldItemFragment::SupportsDeferredExecution() const
{
	return true;
}

bool UPDDropHeldItemFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.IsAuthoritative() ||
		!IsValid(Context.ResolveScopedActor(ApplicationScope)))
	{
		OutError = TEXT("아이템을 드롭시킬 권한 또는 대상 Actor가 없습니다.");
		return false;
	}

	return true;
}

bool UPDDropHeldItemFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	AActor* ScopedActor = Context.ResolveScopedActor(ApplicationScope);
	if (!Context.IsAuthoritative() || !IsValid(ScopedActor))
	{
		return false;
	}

	UPDHeldItemComponent* HeldItemComponent =
		ScopedActor->FindComponentByClass<UPDHeldItemComponent>();
	if (!HeldItemComponent || !HeldItemComponent->HasHeldItem())
	{
		return true;
	}

	return HeldItemComponent->DropHeldItemUsingSettings(
		BuildAdditionalImpulse(Context));
}

FVector UPDDropHeldItemFragment::BuildAdditionalImpulse(
	const FPDActionExecutionContext& Context) const
{
	if (!bApplyImpulse)
	{
		return FVector::ZeroVector;
	}

	return ResolveHorizontalDirection(Context) * HorizontalImpulse +
		FVector::UpVector * VerticalImpulse;
}

FVector UPDDropHeldItemFragment::ResolveHorizontalDirection(
	const FPDActionExecutionContext& Context) const
{
	FVector Direction = FVector::ZeroVector;
	switch (DirectionMode)
	{
	case EPDDropItemImpulseDirectionMode::SourceForward:
		if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EPDDropItemImpulseDirectionMode::InverseHitNormal:
		if (Context.bHasHitResult)
		{
			Direction = -Context.HitResult.ImpactNormal;
		}
		break;

	case EPDDropItemImpulseDirectionMode::SourceToTarget:
	default:
		if (Context.SourceActor && Context.TargetActor)
		{
			Direction = Context.TargetActor->GetActorLocation() -
				Context.SourceActor->GetActorLocation();
		}
		break;
	}

	Direction.Z = 0.0f;
	if (!Direction.Normalize() && Context.SourceActor)
	{
		Direction = Context.SourceActor->GetActorForwardVector();
		Direction.Z = 0.0f;
		Direction.Normalize();
	}
	return Direction;
}
