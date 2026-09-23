#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "PADO/AbilitySystem/Interface/PDAimStateProvider.h"
#include "PADO/Item/Struct/PDItemPresentationStruct.h"
#include "PDAnimInstance.generated.h"

class APDPlayerCharacter;
class UPDCharacterMovementComponent;

/**
 * PADO 캐릭터 AnimBP의 부모다.
 *
 * 매 프레임 캐릭터 상태를 읽어 변수로 펼쳐 둔다. AnimBP는 여기서 값만 꺼내
 * 쓰고 컴포넌트를 직접 타고 들어가지 않는다.
 *
 * Locomotion 변수 이름은 UE 3인칭 템플릿의 `ABP_Unarmed`와 맞췄다.
 * 템플릿 그래프를 옮겨 붙일 때 이름을 고치지 않아도 된다.
 *
 * 값 채우기는 게임 스레드의 NativeUpdateAnimation에서 한다. AnimBP의
 * Thread Safe Update로 옮기려면 여기서 읽는 대상이 모두 워커 스레드에서
 * 안전한지 먼저 확인해야 한다. Held Item과 Ability 상태 조회가 걸려 있어서
 * 지금은 게임 스레드에 둔다.
 */
UCLASS()
class PADO_API UPDAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** 캐릭터 상태를 읽어 아래 변수를 채운다. 파생 클래스가 확장할 수 있다. */
	virtual void RefreshLocomotion();
	virtual void RefreshAim();
	virtual void RefreshHeldItem();

	UPROPERTY(BlueprintReadOnly, Transient, Category = "PD|Owner")
	TObjectPtr<APDPlayerCharacter> OwningCharacter;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "PD|Owner")
	TObjectPtr<UPDCharacterMovementComponent> OwningMovement;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	/** 수평 속력이다. 수직 성분은 빼고 잰다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Locomotion")
	float GroundSpeed = 0.0f;

	/**
	 * 액터 정면 기준 이동 방향이다. -180 ~ 180.
	 * 캐릭터가 카메라 회전을 따르므로 스트레이프 BlendSpace 축으로 바로 쓴다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Locomotion")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Locomotion")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Locomotion")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Locomotion")
	bool bIsSprinting = false;

	/** 이 속력 아래로는 멈춘 것으로 본다. 템플릿과 같은 기본값이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PD|Locomotion",
		meta = (ClampMin = "0.0"))
	float MoveThresholdSpeed = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Aim")
	EPDAimState AimState = EPDAimState::Idle;

	/** Aim Offset용 상하 조준각이다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Aim")
	float AimPitch = 0.0f;

	/** 캐릭터가 컨트롤 회전을 따르므로 평소 0에 가깝다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Aim")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	bool bHasHeldItem = false;

	/** 무기 종류별 자세 키다. 아이템 Definition의 Presentation이 정한다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	EPDHeldPose HeldPose = EPDHeldPose::Default;

	/** 자세 키만으로 나눌 수 없는 예외를 처리할 때 쓴다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	FGameplayTag HeldItemId;
};
