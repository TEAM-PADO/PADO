#include "PADO/Core/PDGameMode.h"

#include "PADO/Core/PDGameState.h"
#include "PADO/Delivery/Definition/PDDeliveryDefinitionSet.h"
#include "PADO/PADO.h"

#include "PADO/Character/PDPlayerState.h"

APDGameMode::APDGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 플레이어의 Ability System은 PlayerState가 소유한다. 다른 클래스를 쓰면
	// 플레이어 캐릭터가 연결할 ASC가 없다.
	PlayerStateClass = APDPlayerState::StaticClass();
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
