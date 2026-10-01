#include "PADO/AbilitySystem/Targeting/PDTargetingCollision.h"

#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"

void PDTargetingCollision::AddIgnoredSourceActors(
	FCollisionQueryParams& QueryParams,
	const AActor& SourceActor,
	const UObject* SourceObject)
{
	QueryParams.AddIgnoredActor(&SourceActor);

	const AActor* SourceObjectActor = Cast<AActor>(SourceObject);
	if (!SourceObjectActor)
	{
		const UActorComponent* SourceComponent =
			Cast<UActorComponent>(SourceObject);
		SourceObjectActor = SourceComponent
			? SourceComponent->GetOwner()
			: nullptr;
	}

	if (SourceObjectActor && SourceObjectActor != &SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceObjectActor);
	}

	if (const AActor* Vehicle = FindSourceVehicle(SourceActor))
	{
		QueryParams.AddIgnoredActor(Vehicle);
	}
}

AActor* PDTargetingCollision::FindSourceVehicle(const AActor& SourceActor)
{
	const APDCharacterBase* Character = Cast<APDCharacterBase>(&SourceActor);
	const UPDVehicleOccupantComponent* Occupant = Character
		? Character->GetVehicleOccupantComponent()
		: nullptr;
	return Occupant ? Occupant->GetCurrentVehicle() : nullptr;
}
