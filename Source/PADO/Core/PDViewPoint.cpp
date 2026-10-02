#include "PADO/Core/PDViewPoint.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

void PDViewPoint::GetActorViewPoint(
	const AActor& Actor,
	FVector& OutLocation,
	FRotator& OutRotation)
{
	const APawn* Pawn = Cast<APawn>(&Actor);
	if (const AController* Controller = Pawn ? Pawn->GetController() : nullptr)
	{
		Controller->GetPlayerViewPoint(OutLocation, OutRotation);
		return;
	}

	Actor.GetActorEyesViewPoint(OutLocation, OutRotation);
}
