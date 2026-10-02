#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"

UPDVehicleSeatComponent::UPDVehicleSeatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPDVehicleSeatComponent::ConfigureSeat(
	int32 NewSeatNumber,
	EPDVehicleSeatRole NewSeatRole)
{
	SeatNumber = FMath::Max(1, NewSeatNumber);
	SeatRole = NewSeatRole;
}

FVector UPDVehicleSeatComponent::GetExitLocation() const
{
	return GetComponentTransform().TransformPosition(ExitOffset);
}
