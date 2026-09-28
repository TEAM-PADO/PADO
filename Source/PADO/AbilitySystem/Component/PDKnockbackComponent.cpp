#include "PADO/AbilitySystem/Component/PDKnockbackComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"

bool UPDKnockbackComponent::CanApplyKnockback(
	const FPDKnockbackRequest& Request) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() ||
		Request.Direction.ContainsNaN() ||
		!FMath::IsFinite(Request.HorizontalSpeed) ||
		!FMath::IsFinite(Request.VerticalSpeed) ||
		Request.HorizontalSpeed < 0.0f ||
		Request.VerticalSpeed < 0.0f ||
		(FMath::IsNearlyZero(Request.HorizontalSpeed) &&
			FMath::IsNearlyZero(Request.VerticalSpeed)))
	{
		return false;
	}

	if (!FMath::IsNearlyZero(Request.HorizontalSpeed))
	{
		FVector HorizontalDirection = Request.Direction;
		HorizontalDirection.Z = 0.0f;
		if (!HorizontalDirection.Normalize())
		{
			return false;
		}
	}

	if (Owner->IsA<ACharacter>())
	{
		return true;
	}

	const UPrimitiveComponent* PrimitiveComponent = ResolvePhysicsComponent();
	return PrimitiveComponent && PrimitiveComponent->IsSimulatingPhysics();
}

bool UPDKnockbackComponent::ApplyKnockback(
	const FPDKnockbackRequest& Request)
{
	AActor* Owner = GetOwner();
	if (!CanApplyKnockback(Request))
	{
		return false;
	}

	FVector LaunchVelocity = FVector::UpVector * Request.VerticalSpeed;
	FVector HorizontalDirection = Request.Direction;
	HorizontalDirection.Z = 0.0f;
	if (!FMath::IsNearlyZero(Request.HorizontalSpeed))
	{
		HorizontalDirection.Normalize();
		LaunchVelocity += HorizontalDirection * Request.HorizontalSpeed;
	}

	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		Character->LaunchCharacter(
			LaunchVelocity,
			Request.bOverrideHorizontalVelocity,
			Request.bOverrideVerticalVelocity);
		return true;
	}

	if (UPrimitiveComponent* PrimitiveComponent = ResolvePhysicsComponent())
	{
		PrimitiveComponent->AddImpulse(
			LaunchVelocity,
			NAME_None,
			true);
		return true;
	}

	return false;
}

UPrimitiveComponent* UPDKnockbackComponent::ResolvePhysicsComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UActorComponent* ReferencedComponent =
		PhysicsComponent.GetComponent(Owner))
	{
		return Cast<UPrimitiveComponent>(ReferencedComponent);
	}

	return Cast<UPrimitiveComponent>(Owner->GetRootComponent());
}
