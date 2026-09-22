// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDPlayerController.h"

#include "PDPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

void APDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!ensureMsgf(DefaultMappingContext && MoveAction && LookAction && JumpAction && SprintAction && InteractAction && AttackAction && ReloadAction,
		TEXT("PD player input setup is incomplete. Configure every input asset on the PlayerController class defaults.")))
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensureMsgf(EnhancedInputComponent, TEXT("PADO requires an Enhanced Input component.")))
	{
		return;
	}

	AddDefaultMappingContext();

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMove);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLook);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::HandleJumpStarted);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::HandleJumpCompleted);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::HandleJumpCompleted);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::HandleSprintStarted);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::HandleSprintCompleted);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::HandleSprintCompleted);
	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::HandleInteract);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::HandleAttackStarted);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &ThisClass::HandleAttackCompleted);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Canceled, this, &ThisClass::HandleAttackCompleted);
	EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::HandleReload);

	// Drop은 선택 입력이다. 에셋을 지정하지 않으면 바인딩만 건너뛴다.
	if (DropAction)
	{
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &ThisClass::HandleDrop);
	}
}

void APDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveDefaultMappingContext();
	Super::EndPlay(EndPlayReason);
}

void APDPlayerController::AddDefaultMappingContext()
{
	if (bDefaultMappingContextAdded || !DefaultMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
		bDefaultMappingContextAdded = true;
	}
}

void APDPlayerController::RemoveDefaultMappingContext()
{
	if (!bDefaultMappingContextAdded || !DefaultMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		bDefaultMappingContextAdded = false;
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		Subsystem->RemoveMappingContext(DefaultMappingContext);
	}

	bDefaultMappingContextAdded = false;
}

APDPlayerCharacter* APDPlayerController::GetPDPlayerCharacter() const
{
	return Cast<APDPlayerCharacter>(GetPawn());
}

void APDPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->Move(Value.Get<FVector2D>());
	}
}

void APDPlayerController::HandleLook(const FInputActionValue& Value)
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->Look(Value.Get<FVector2D>());
	}
}

void APDPlayerController::HandleJumpStarted()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StartJump();
	}
}

void APDPlayerController::HandleJumpCompleted()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StopJump();
	}
}

void APDPlayerController::HandleSprintStarted()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StartSprinting();
	}
}

void APDPlayerController::HandleSprintCompleted()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StopSprinting();
	}
}

void APDPlayerController::HandleInteract()
{
	OnInteractRequested.Broadcast();

	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->Interact();
	}
}

void APDPlayerController::HandleAttackStarted()
{
	OnAttackRequested.Broadcast();

	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StartAttacking();
	}
}

void APDPlayerController::HandleAttackCompleted()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StopAttacking();
	}
}

void APDPlayerController::HandleReload()
{
	OnReloadRequested.Broadcast();

	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->ReloadHeldItem();
	}
}

void APDPlayerController::HandleDrop()
{
	OnDropRequested.Broadcast();

	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->DropHeldItem();
	}
}
