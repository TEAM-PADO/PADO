#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

namespace PDMovementAttributeDefaults
{
	/** UE 3인칭 기본 걷기 속도와 같은 값이다. */
	constexpr float MoveSpeed = 500.0f;
}

UPDMovementAttributeSet::UPDMovementAttributeSet()
	: MoveSpeed(PDMovementAttributeDefaults::MoveSpeed)
{
}

void UPDMovementAttributeSet::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 이동 속도는 관찰자의 보간에도 영향을 주므로 모두에게 보낸다.
	DOREPLIFETIME_CONDITION_NOTIFY(
		UPDMovementAttributeSet,
		MoveSpeed,
		COND_None,
		REPNOTIFY_Always);
}

void UPDMovementAttributeSet::PreAttributeChange(
	const FGameplayAttribute& Attribute,
	float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 음수 속도는 뒤로 걷는 것이 아니라 이동 자체를 망가뜨린다.
	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void UPDMovementAttributeSet::PreAttributeBaseChange(
	const FGameplayAttribute& Attribute,
	float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void UPDMovementAttributeSet::OnRep_MoveSpeed(
	const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPDMovementAttributeSet, MoveSpeed, OldMoveSpeed);
}
