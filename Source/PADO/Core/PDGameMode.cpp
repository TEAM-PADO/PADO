#include "PADO/Core/PDGameMode.h"

#include "PADO/Core/PDGameState.h"
#include "PADO/Delivery/Definition/PDDeliveryDefinitionSet.h"
#include "PADO/PADO.h"

APDGameMode::APDGameMode()
{
	GameStateClass = APDGameState::StaticClass();
}

UPDDeliveryDefinitionSet* APDGameMode::GetDeliveryDefinitionSet() const
{
	return DeliveryDefinitionSet;
}

void APDGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!DeliveryDefinitionSet)
	{
		UE_LOG(LogPDServer, Error, TEXT("DeliveryDefinitionSet is not configured. Delivery candidates and acceptance cannot be processed."));
		return;
	}

	FString ValidationError;
	if (!DeliveryDefinitionSet->Validate(ValidationError))
	{
		UE_LOG(LogPDServer, Error, TEXT("DeliveryDefinitionSet '%s' validation failed: %s"), *DeliveryDefinitionSet->GetName(), *ValidationError);
	}
}
