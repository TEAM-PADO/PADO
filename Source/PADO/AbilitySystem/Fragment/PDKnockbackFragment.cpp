#include "PADO/AbilitySystem/Fragment/PDKnockbackFragment.h"

#include "PADO/AbilitySystem/Component/PDKnockbackComponent.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "GameFramework/Actor.h"

bool UPDKnockbackFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(HorizontalSpeed) || HorizontalSpeed < 0.0f)
	{
		OutError = TEXT("HorizontalSpeed는 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(VerticalSpeed) || VerticalSpeed < 0.0f)
	{
		OutError = TEXT("VerticalSpeed는 0 이상의 유한한 수여야 합니다.");
		return false;
	}

	if (FMath::IsNearlyZero(HorizontalSpeed) &&
		FMath::IsNearlyZero(VerticalSpeed))
	{
		OutError = TEXT("넉백의 수평·수직 속도가 모두 0입니다.");
		return false;
	}

	return true;
}

bool UPDKnockbackFragment::SupportsDeferredExecution() const
{
	return true;
}

bool UPDKnockbackFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	AActor* ScopedActor = Context.ResolveScopedActor(ApplicationScope);
	if (!Context.IsAuthoritative() || !IsValid(ScopedActor))
	{
		OutError = TEXT("넉백을 적용할 권한 또는 대상 Actor가 없습니다.");
		return false;
	}

	UPDKnockbackComponent* KnockbackComponent =
		ScopedActor->FindComponentByClass<UPDKnockbackComponent>();
	FPDKnockbackRequest Request;
	if (!KnockbackComponent || !BuildRequest(Context, Request) ||
		!KnockbackComponent->CanApplyKnockback(Request))
	{
		OutError = TEXT("Target이 현재 넉백 요청을 처리할 수 없습니다.");
		return false;
	}

	return true;
}

bool UPDKnockbackFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	AActor* ScopedActor = Context.ResolveScopedActor(ApplicationScope);
	if (!Context.IsAuthoritative() || !IsValid(ScopedActor))
	{
		return false;
	}

	UPDKnockbackComponent* KnockbackComponent =
		ScopedActor->FindComponentByClass<UPDKnockbackComponent>();
	if (!KnockbackComponent)
	{
		return false;
	}

	FPDKnockbackRequest Request;
	return BuildRequest(Context, Request) &&
		KnockbackComponent->ApplyKnockback(Request);
}

bool UPDKnockbackFragment::BuildRequest(
	const FPDActionExecutionContext& Context,
	FPDKnockbackRequest& OutRequest) const
{
	const FVector Direction = ResolveDirection(Context);
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OutRequest.Direction = Direction;
	OutRequest.HorizontalSpeed = HorizontalSpeed;
	OutRequest.VerticalSpeed = VerticalSpeed;
	OutRequest.bOverrideHorizontalVelocity = bOverrideHorizontalVelocity;
	OutRequest.bOverrideVerticalVelocity = bOverrideVerticalVelocity;
	return true;
}

FVector UPDKnockbackFragment::ResolveDirection(
	const FPDActionExecutionContext& Context) const
{
	FVector Direction = FVector::ZeroVector;
	switch (DirectionMode)
	{
	case EPDKnockbackDirectionMode::SourceForward:
		if (Context.SourceActor)
		{
			Direction = Context.SourceActor->GetActorForwardVector();
		}
		break;

	case EPDKnockbackDirectionMode::InverseHitNormal:
		if (Context.bHasHitResult)
		{
			Direction = -Context.HitResult.ImpactNormal;
		}
		break;

	case EPDKnockbackDirectionMode::SourceToTarget:
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
