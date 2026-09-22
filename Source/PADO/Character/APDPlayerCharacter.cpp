// Copyright Epic Games, Inc. All Rights Reserved.

#include "APDPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

namespace PDCharacterDefaults
{
	constexpr float CapsuleRadius = 42.0f;
	constexpr float CapsuleHalfHeight = 96.0f;
	constexpr float RotationRateYaw = 500.0f;
	constexpr float JumpZVelocity = 500.0f;
	constexpr float AirControl = 0.35f;
	constexpr float MaxWalkSpeed = 500.0f;
	constexpr float MinAnalogWalkSpeed = 20.0f;
	constexpr float BrakingDecelerationWalking = 2000.0f;
	constexpr float BrakingDecelerationFalling = 1500.0f;
	constexpr float CameraBoomLength = 400.0f;
}

APDPlayerCharacter::APDPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(PDCharacterDefaults::CapsuleRadius, PDCharacterDefaults::CapsuleHalfHeight);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->RotationRate = FRotator(0.0f, PDCharacterDefaults::RotationRateYaw, 0.0f);
	MovementComponent->JumpZVelocity = PDCharacterDefaults::JumpZVelocity;
	MovementComponent->AirControl = PDCharacterDefaults::AirControl;
	MovementComponent->MaxWalkSpeed = PDCharacterDefaults::MaxWalkSpeed;
	MovementComponent->MinAnalogWalkSpeed = PDCharacterDefaults::MinAnalogWalkSpeed;
	MovementComponent->BrakingDecelerationWalking = PDCharacterDefaults::BrakingDecelerationWalking;
	MovementComponent->BrakingDecelerationFalling = PDCharacterDefaults::BrakingDecelerationFalling;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = PDCharacterDefaults::CameraBoomLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APDPlayerCharacter::Move(const FVector2D& MovementInput)
{
	if (!Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

void APDPlayerCharacter::Look(const FVector2D& LookInput)
{
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void APDPlayerCharacter::StartJump()
{
	Jump();
}

void APDPlayerCharacter::StopJump()
{
	StopJumping();
}

void APDPlayerCharacter::StartSprinting_Implementation()
{
	// Intentionally empty. Sprinting requires an authoritative, predicted movement implementation.
}

void APDPlayerCharacter::StopSprinting_Implementation()
{
	// Intentionally empty. Sprinting requires an authoritative, predicted movement implementation.
}

void APDPlayerCharacter::Interact_Implementation()
{
	// Intentionally empty. Implement interaction in a derived C++ or Blueprint class.
}

void APDPlayerCharacter::Attack_Implementation()
{
	// Intentionally empty. Implement combat in a derived C++ or Blueprint class.
}
