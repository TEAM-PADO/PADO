// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "PDCharacterBase.generated.h"

class UAbilitySystemComponent;
class UPDAbilitySystemComponent;
class UPDCharacterMovementComponent;
class UPDHeldItemComponent;
class UPDInteractionComponent;
class UPDKnockbackComponent;
class UPDVehicleOccupantComponent;
struct FOnAttributeChangeData;

/**
 * PADO 캐릭터가 공유하는 몸이다.
 *
 * 플레이어와 유사한 캐릭터가 똑같이 반응해야 하는 것(Ability System 연결,
 * 이동 속도 Attribute, 넉백, 아이템 보유)을 여기에 둔다. 카메라·조준·입력처럼
 * 조종하는 사람에게만 의미가 있는 것은 파생 클래스가 가진다.
 *
 * ASC가 어디 있는지는 가정하지 않는다. 플레이어는 PlayerState가 소유하고,
 * 플레이어가 아닌 캐릭터는 자기 자신이 소유한다. 파생 클래스가 자기 초기화
 * 경계에서 InitializeAbilitySystem으로 이 몸을 아바타로 연결한다.
 */
UCLASS(Abstract)
class PADO_API APDCharacterBase
	: public ACharacter
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	explicit APDCharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * 이 몸을 아바타로 ASC에 연결한다.
	 *
	 * 초기화 경계마다 여러 번 불려도 안전하다. 다른 ASC로 바뀌면 이전 연결을
	 * 먼저 풀고, 같은 ASC를 다른 몸이 아직 아바타로 쥐고 있으면 그 몸을 내보낸다.
	 */
	void InitializeAbilitySystem(
		UPDAbilitySystemComponent* InAbilitySystem,
		AActor* InOwnerActor);

	UFUNCTION(BlueprintPure, Category = "PADO|Ability")
	UPDAbilitySystemComponent* GetPDAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category = "PADO|Item")
	UPDHeldItemComponent* GetHeldItemComponent() const
	{
		return HeldItemComponent;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Interaction")
	UPDInteractionComponent* GetInteractionComponent() const
	{
		return InteractionComponent;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Vehicle")
	UPDVehicleOccupantComponent* GetVehicleOccupantComponent() const
	{
		return VehicleOccupantComponent;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Movement")
	UPDCharacterMovementComponent* GetPDCharacterMovement() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * 이 몸과 ASC의 연결을 푼다. 아직 이 몸이 아바타면 활성 실행을 끝내고
	 * 아바타를 비운다. 다른 몸이 이미 넘겨받았다면 그쪽 연결은 건드리지 않는다.
	 */
	void UninitializeAbilitySystem();

	void PushMoveSpeedToMovement();
	void HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Item", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDHeldItemComponent> HeldItemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Ability", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDKnockbackComponent> KnockbackComponent;

	/** 상호작용 대상을 고르고 서버에 요청한다. 손 닿는 거리를 가진다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDInteractionComponent> InteractionComponent;

	/** 탈것에 탄 상태다. 앉으면 몸을 좌석에 붙이고 이동과 충돌을 끈다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDVehicleOccupantComponent> VehicleOccupantComponent;

	/** 연결된 ASC다. 이 몸이 소유하지 않을 수 있으므로 약참조로 둔다. */
	TWeakObjectPtr<UPDAbilitySystemComponent> AbilitySystem;

	FDelegateHandle MoveSpeedChangedHandle;
};
