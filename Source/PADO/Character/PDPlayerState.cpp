// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDPlayerState.h"

#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"

namespace PDPlayerStateDefaults
{
	/**
	 * PlayerState 기본값은 초당 1회다. ASC의 Attribute와 Gameplay Effect가
	 * 여기로 복제되므로 그대로 두면 최대 1초 늦게 도착한다. Lyra와 같은 값이다.
	 */
	constexpr float NetUpdateFrequency = 100.0f;
}

APDPlayerState::APDPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetNetUpdateFrequency(PDPlayerStateDefaults::NetUpdateFrequency);

	AbilitySystemComponent =
		CreateDefaultSubobject<UPDAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	MovementAttributes =
		CreateDefaultSubobject<UPDMovementAttributeSet>(TEXT("MovementAttributes"));
}

UAbilitySystemComponent* APDPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
