// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "PDPlayerCharacter.generated.h"

class APDWorldItemActor;
class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UPDAbilitySystemComponent;
class UPDHeldItemComponent;
class UPDKnockbackComponent;

/**
 * Base third-person player character.
 *
 * Input is owned and bound by APDPlayerController. The controller forwards
 * gameplay intent to the public functions on this class.
 */
UCLASS(Blueprintable)
class PADO_API APDPlayerCharacter
	: public ACharacter
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APDPlayerCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

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

	/**
	 * 시선 앞의 월드 아이템을 집는다.
	 * 이미 아이템을 들고 있으면 아무것도 하지 않는다. 먼저 내려놓아야 한다.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PADO|Input")
	void Interact();
	virtual void Interact_Implementation();

	/** 현재 들고 있는 아이템을 내려놓는다. 서버가 확정한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	void DropHeldItem();

	/**
	 * 카메라 시선을 따라 Sweep해서 지금 집을 수 있는 아이템을 고른다.
	 * 시선에 걸린 것이 없으면 줍기 반경 안의 가장 가까운 아이템으로 넘어간다.
	 * 실제 줍기 거리는 Held Item Component가 서버에서 확정한다.
	 */
	UFUNCTION(BlueprintPure, Category = "PADO|Interaction")
	APDWorldItemActor* FindInteractTarget() const;

	/** 줍기 반경 안에서 캐릭터와 가장 가까운 아이템을 고른다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Interaction")
	APDWorldItemActor* FindNearestPickupCandidate() const;

	/** Extension point for the combat system that will be added later. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PADO|Input")
	void Attack();
	virtual void Attack_Implementation();

	/** 공격 입력 Press를 현재 Held Item에 전달하고, 사용할 아이템이 없으면 Attack 이벤트를 호출한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	bool StartAttacking();

	/** Press 당시 Held Item의 동일한 Ability Spec에 Release를 전달한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	void StopAttacking();

	/** 현재 Held Item이 Reloadable이면 서버 권한 재장전을 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	bool ReloadHeldItem();

	UFUNCTION(BlueprintPure, Category = "PADO|Ability")
	UPDAbilitySystemComponent* GetPDAbilitySystemComponent() const
	{
		return AbilitySystemComponent;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Item")
	UPDHeldItemComponent* GetHeldItemComponent() const
	{
		return HeldItemComponent;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Camera")
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintPure, Category = "PADO|Camera")
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void PawnClientRestart() override;

private:
	void InitializeAbilityActorInfo();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Ability", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Item", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDHeldItemComponent> HeldItemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Ability", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDKnockbackComponent> KnockbackComponent;

	/** 시선 Sweep의 굵기다. 크게 잡을수록 작은 아이템을 조준하기 쉽다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PADO|Interaction",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractTraceRadius = 24.0f;

	/**
	 * 카메라에서 앞으로 탐색할 거리다. 3인칭 카메라가 캐릭터 뒤에 있으므로
	 * 실제 줍기 반경보다 길게 잡는다. 거리 판정 자체는 서버가 캐릭터 기준으로 한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PADO|Interaction",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float InteractTraceDistance = 900.0f;
};
