// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "PDPlayerState.generated.h"

class UAbilitySystemComponent;
class UPDAbilitySystemComponent;
class UPDHealthAttributeSet;
class UPDMovementAttributeSet;

/**
 * 플레이어의 Ability System을 소유한다.
 *
 * ASC를 몸이 아니라 여기에 두는 이유는 몸이 바뀌거나 죽어도 플레이어 단위
 * 상태를 이어 가기 위해서다. 몸은 APDCharacterBase가 아바타로 연결한다.
 * PlayerState는 세션 내내 PlayerController가 소유하므로 Mixed 복제 조건
 * (OwnerActor의 Owner가 Controller)을 만족한다.
 */
UCLASS()
class PADO_API APDPlayerState
	: public APlayerState
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	explicit APDPlayerState(const FObjectInitializer& ObjectInitializer);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "PADO|Ability")
	UPDAbilitySystemComponent* GetPDAbilitySystemComponent() const
	{
		return AbilitySystemComponent;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Movement")
	const UPDMovementAttributeSet* GetMovementAttributes() const
	{
		return MovementAttributes;
	}

	UFUNCTION(BlueprintPure, Category = "PADO|Health")
	const UPDHealthAttributeSet* GetHealthAttributes() const
	{
		return HealthAttributes;
	}

	/**
	 * 서버에서 새 몸으로 다시 시작할 때 부른다. 걸려 있던 Gameplay Effect(슬로우·헤이스트,
	 * 쿨다운 등)를 모두 지우고 체력을 최대로 채운다.
	 */
	void ResetForRespawn();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Ability", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDAbilitySystemComponent> AbilitySystemComponent;

	/**
	 * 슬로우·헤이스트가 붙는 계층이다. 이 액터의 서브오브젝트라서 ASC가
	 * 초기화할 때 자동으로 수집한다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDMovementAttributeSet> MovementAttributes;

	/** 체력이다. 몸이 바뀌어도 플레이어 단위로 남는다. 새 몸의 체력을 채우는 것은 리스폰이 정한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDHealthAttributeSet> HealthAttributes;
};
