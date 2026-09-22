// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "APDPlayerController.generated.h"

class APDPlayerCharacter;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDInputRequestedSignature);

/**
 * Owns gameplay input bindings and forwards player intent to the currently
 * possessed APDPlayerCharacter.
 */
UCLASS(Blueprintable)
class PADO_API APDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Broadcast before forwarding the interaction input to the character. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Input")
	FPDInputRequestedSignature OnInteractRequested;

	/** Broadcast before forwarding the attack input to the character. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Input")
	FPDInputRequestedSignature OnAttackRequested;

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetMoveAction() const { return MoveAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetLookAction() const { return LookAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetJumpAction() const { return JumpAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetSprintAction() const { return SprintAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetInteractAction() const { return InteractAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetAttackAction() const { return AttackAction; }

protected:
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Priority used when registering the player mapping context with Enhanced Input. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input")
	int32 DefaultMappingPriority = 0;

private:
	void AddDefaultMappingContext();
	void RemoveDefaultMappingContext();

	APDPlayerCharacter* GetPDPlayerCharacter() const;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpCompleted();
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleInteract();
	void HandleAttack();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AttackAction;

	bool bDefaultMappingContextAdded = false;
};
