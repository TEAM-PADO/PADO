// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PDPlayerController.generated.h"

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

	/** Broadcast before forwarding the reload input to the character. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Input")
	FPDInputRequestedSignature OnReloadRequested;

	/** Broadcast before forwarding the drop input to the character. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Input")
	FPDInputRequestedSignature OnDropRequested;

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

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetReloadAction() const { return ReloadAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetDropAction() const { return DropAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetAimHoldAction() const { return AimHoldAction; }

	UFUNCTION(BlueprintPure, Category = "PADO|Input")
	UInputAction* GetAimToggleAction() const { return AimToggleAction; }

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
	void HandleAttackStarted();
	void HandleAttackCompleted();
	void HandleReload();
	void HandleDrop();
	void HandleAimHoldTriggered();
	void HandleAimHoldCompleted();
	void HandleAimToggle();

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ReloadAction;

	/** 선택 입력이다. 비워 두면 드롭 조작을 바인딩하지 않고 나머지 입력은 정상 동작한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> DropAction;

	/** 선택 입력이다. Hold 트리거를 붙여 견착에 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AimHoldAction;

	/** 선택 입력이다. Tap 트리거를 붙여 조준 토글에 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AimToggleAction;

	bool bDefaultMappingContextAdded = false;
};
