#include "PADO/Character/PDCharacterMovementComponent.h"

#include "GameFramework/Character.h"

namespace PDMovementFlags
{
	/** 조준 단계 2비트다. 값이 3이면 잘못된 조합이므로 Aiming으로 잘라 쓴다. */
	constexpr uint8 AimLow = FSavedMove_Character::FLAG_Custom_0;
	constexpr uint8 AimHigh = FSavedMove_Character::FLAG_Custom_1;
	constexpr uint8 Sprint = FSavedMove_Character::FLAG_Custom_2;

	uint8 PackAimState(EPDAimState AimState)
	{
		const uint8 Bits = static_cast<uint8>(AimState);
		uint8 Flags = 0;
		if (Bits & 1)
		{
			Flags |= AimLow;
		}
		if (Bits & 2)
		{
			Flags |= AimHigh;
		}
		return Flags;
	}

	EPDAimState UnpackAimState(uint8 Flags)
	{
		const uint8 Bits =
			((Flags & AimLow) != 0 ? 1 : 0) |
			((Flags & AimHigh) != 0 ? 2 : 0);
		return static_cast<EPDAimState>(
			FMath::Min<uint8>(Bits, static_cast<uint8>(EPDAimState::Aiming)));
	}
}

UPDCharacterMovementComponent::UPDCharacterMovementComponent()
{
	bWantsToSprint = 0;
}

float UPDCharacterMovementComponent::GetMaxSpeed() const
{
	// 걷기 외 모드와 웅크리기는 엔진 규칙을 그대로 쓴다.
	if ((MovementMode != MOVE_Walking && MovementMode != MOVE_NavWalking) ||
		IsCrouching())
	{
		return Super::GetMaxSpeed();
	}

	return FMath::Max(0.0f, AttributeMoveSpeed * GetStanceSpeedMultiplier());
}

float UPDCharacterMovementComponent::GetStanceSpeedMultiplier() const
{
	// 조준 자세가 스프린트보다 우선한다. 조준하면서 달리지는 않는다.
	switch (AimState)
	{
	case EPDAimState::Shouldered:
		return ShoulderedSpeedMultiplier;

	case EPDAimState::Aiming:
		return AimingSpeedMultiplier;

	case EPDAimState::Idle:
	default:
		break;
	}

	return WantsToSprint() ? SprintSpeedMultiplier : 1.0f;
}

bool UPDCharacterMovementComponent::CanSprint() const
{
	return AimState == EPDAimState::Idle && !IsCrouching() &&
		(MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
}

bool UPDCharacterMovementComponent::IsSprinting() const
{
	return WantsToSprint() && CanSprint();
}

void UPDCharacterMovementComponent::SetWantsToSprint(bool bNewWantsToSprint)
{
	bWantsToSprint = bNewWantsToSprint ? 1 : 0;
}

void UPDCharacterMovementComponent::SetAimState(EPDAimState NewAimState)
{
	AimState = NewAimState;
}

void UPDCharacterMovementComponent::SetAttributeMoveSpeed(float NewMoveSpeed)
{
	AttributeMoveSpeed = FMath::Max(0.0f, NewMoveSpeed);
}

void UPDCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToSprint = (Flags & PDMovementFlags::Sprint) != 0 ? 1 : 0;
	AimState = PDMovementFlags::UnpackAimState(Flags);
}

FNetworkPredictionData_Client*
UPDCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UPDCharacterMovementComponent* MutableThis =
			const_cast<UPDCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData =
			new FPDNetworkPredictionData_Client(*this);
	}

	return ClientPredictionData;
}

void FPDSavedMove::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = 0;
	SavedAimState = EPDAimState::Idle;
}

uint8 FPDSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= PDMovementFlags::Sprint;
	}
	Result |= PDMovementFlags::PackAimState(SavedAimState);
	return Result;
}

bool FPDSavedMove::CanCombineWith(
	const FSavedMovePtr& NewMove,
	ACharacter* InCharacter,
	float MaxDelta) const
{
	// 자세가 다른 두 이동을 합치면 서버가 재생할 때 구간이 뭉개진다.
	const FPDSavedMove* Other = static_cast<const FPDSavedMove*>(NewMove.Get());
	if (Other &&
		(bSavedWantsToSprint != Other->bSavedWantsToSprint ||
			SavedAimState != Other->SavedAimState))
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FPDSavedMove::SetMoveFor(
	ACharacter* C,
	float InDeltaTime,
	const FVector& NewAccel,
	FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UPDCharacterMovementComponent* Movement =
		C ? Cast<UPDCharacterMovementComponent>(C->GetCharacterMovement()) : nullptr)
	{
		bSavedWantsToSprint = Movement->WantsToSprint() ? 1 : 0;
		SavedAimState = Movement->GetAimState();
	}
}

void FPDSavedMove::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	if (UPDCharacterMovementComponent* Movement =
		C ? Cast<UPDCharacterMovementComponent>(C->GetCharacterMovement()) : nullptr)
	{
		Movement->SetWantsToSprint(bSavedWantsToSprint != 0);
		Movement->SetAimState(SavedAimState);
	}
}

FPDNetworkPredictionData_Client::FPDNetworkPredictionData_Client(
	const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FPDNetworkPredictionData_Client::AllocateNewMove()
{
	return MakeShared<FPDSavedMove>();
}
