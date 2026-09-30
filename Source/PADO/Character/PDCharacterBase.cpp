// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDCharacterBase.h"

#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Component/PDKnockbackComponent.h"
#include "PADO/Character/PDCharacterMovementComponent.h"
#include "PADO/Interaction/Component/PDInteractionComponent.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"

APDCharacterBase::APDCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPDCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	// 서브오브젝트 이름은 파생 Blueprint에 저장된 컴포넌트 값의 키다. 바꾸지 않는다.
	HeldItemComponent =
		CreateDefaultSubobject<UPDHeldItemComponent>(TEXT("HeldItem"));
	KnockbackComponent =
		CreateDefaultSubobject<UPDKnockbackComponent>(TEXT("Knockback"));
	InteractionComponent =
		CreateDefaultSubobject<UPDInteractionComponent>(TEXT("Interaction"));
}

UAbilitySystemComponent* APDCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystem.Get();
}

UPDAbilitySystemComponent* APDCharacterBase::GetPDAbilitySystemComponent() const
{
	return AbilitySystem.Get();
}

UPDCharacterMovementComponent* APDCharacterBase::GetPDCharacterMovement() const
{
	return Cast<UPDCharacterMovementComponent>(GetCharacterMovement());
}

void APDCharacterBase::InitializeAbilitySystem(
	UPDAbilitySystemComponent* InAbilitySystem,
	AActor* InOwnerActor)
{
	if (!InAbilitySystem || !InOwnerActor)
	{
		return;
	}

	if (AbilitySystem.Get() != InAbilitySystem)
	{
		UninitializeAbilitySystem();
		AbilitySystem = InAbilitySystem;
	}

	// 같은 PlayerState로 새 몸이 먼저 도착하면 이전 몸이 아직 아바타일 수 있다.
	// 클라이언트에서는 이전 몸의 파괴가 복제로 늦게 온다. 이전 몸의 연결부터 푼다.
	APDCharacterBase* PreviousAvatar =
		Cast<APDCharacterBase>(InAbilitySystem->GetAvatarActor());
	if (PreviousAvatar && PreviousAvatar != this)
	{
		PreviousAvatar->UninitializeAbilitySystem();
	}

	InAbilitySystem->InitAbilityActorInfo(InOwnerActor, this);

	// 이 함수는 초기화 경계마다 불린다. 같은 ASC에 두 번 등록하지 않는다.
	if (!MoveSpeedChangedHandle.IsValid())
	{
		MoveSpeedChangedHandle = InAbilitySystem
			->GetGameplayAttributeValueChangeDelegate(
				UPDMovementAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &APDCharacterBase::HandleMoveSpeedChanged);
	}

	PushMoveSpeedToMovement();
}

void APDCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컴포넌트 정리가 먼저다. 들고 있던 아이템은 Held Item Component가 내려놓으며
	// 부여했던 Ability를 회수한다. 그 뒤에 아바타 연결을 푼다.
	Super::EndPlay(EndPlayReason);
	UninitializeAbilitySystem();
}

void APDCharacterBase::UninitializeAbilitySystem()
{
	UPDAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get();
	AbilitySystem.Reset();
	if (!CurrentAbilitySystem)
	{
		MoveSpeedChangedHandle.Reset();
		return;
	}

	if (MoveSpeedChangedHandle.IsValid())
	{
		CurrentAbilitySystem
			->GetGameplayAttributeValueChangeDelegate(
				UPDMovementAttributeSet::GetMoveSpeedAttribute())
			.Remove(MoveSpeedChangedHandle);
		MoveSpeedChangedHandle.Reset();
	}

	if (CurrentAbilitySystem->GetAvatarActor() != this)
	{
		return;
	}

	// 몸 없이 이어 갈 실행은 없다. Spec은 남기고 활성 실행만 끝낸다.
	// 사망 시 무엇을 남길지는 피해 시스템에서 "누가 부여했나"로 따로 정한다.
	CurrentAbilitySystem->CancelAllAbilities();
	if (CurrentAbilitySystem->GetOwnerActor())
	{
		CurrentAbilitySystem->SetAvatarActor(nullptr);
	}
	else
	{
		CurrentAbilitySystem->ClearActorInfo();
	}
}

void APDCharacterBase::PushMoveSpeedToMovement()
{
	const UPDAbilitySystemComponent* CurrentAbilitySystem = AbilitySystem.Get();
	UPDCharacterMovementComponent* Movement = GetPDCharacterMovement();
	if (!CurrentAbilitySystem || !Movement)
	{
		return;
	}

	// Attribute Set이 없는 ASC는 0을 돌려준다. 그대로 넣으면 움직일 수 없으므로
	// 이 경우에는 무브먼트의 기본 속도를 둔다.
	bool bFound = false;
	const float MoveSpeed = CurrentAbilitySystem->GetGameplayAttributeValue(
		UPDMovementAttributeSet::GetMoveSpeedAttribute(),
		bFound);
	if (bFound)
	{
		Movement->SetAttributeMoveSpeed(MoveSpeed);
	}
}

void APDCharacterBase::HandleMoveSpeedChanged(
	const FOnAttributeChangeData& ChangeData)
{
	if (UPDCharacterMovementComponent* Movement = GetPDCharacterMovement())
	{
		Movement->SetAttributeMoveSpeed(ChangeData.NewValue);
	}
}
