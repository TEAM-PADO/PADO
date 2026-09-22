// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "APDPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Base third-person player character.
 *
 * Input is owned and bound by APDPlayerController. The controller forwards
 * gameplay intent to the public functions on this class.
 */
UCLASS(Blueprintable)
class PADO_API APDPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APDPlayerCharacter();

	/** Applies camera-relative movement input. X is right and Y is forward. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	virtual void Move(const FVector2D& MovementInput);

	/** Applies yaw/pitch input to the controller. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	virtual void Look(const FVector2D& LookInput);

	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	virtual void StartJump();

	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	virtual void StopJump();

	/** Extension point for a future network-aware sprint implementation. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PADO|Input")
	void StartSprinting();
	virtual void StartSprinting_Implementation();

	/** Extension point for a future network-aware sprint implementation. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PADO|Input")
	void StopSprinting();
	virtual void StopSprinting_Implementation();

	/** Extension point for the interaction system that will be added later. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PADO|Input")
	void Interact();
	virtual void Interact_Implementation();

	/** Extension point for the combat system that will be added later. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PADO|Input")
	void Attack();
	virtual void Attack_Implementation();

	UFUNCTION(BlueprintPure, Category = "PADO|Camera")
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintPure, Category = "PADO|Camera")
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
};
