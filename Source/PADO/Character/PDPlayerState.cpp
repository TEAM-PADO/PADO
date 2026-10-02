// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDPlayerState.h"

#include "GameplayEffect.h"
#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"
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
	HealthAttributes =
		CreateDefaultSubobject<UPDHealthAttributeSet>(TEXT("HealthAttributes"));
}

UAbilitySystemComponent* APDPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APDPlayerState::ResetForRespawn()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	// 지난 몸에 걸려 있던 효과를 모두 지운다. 조건이 빈 Query는 모든 효과와 맞는다.
	// 죽어도 남아야 하는 플레이어 효과가 생기면 여기서 제외 규칙을 둔다.
	AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());

	if (HealthAttributes)
	{
		AbilitySystemComponent->SetNumericAttributeBase(
			UPDHealthAttributeSet::GetHealthAttribute(),
			HealthAttributes->GetMaxHealth());
	}
}
