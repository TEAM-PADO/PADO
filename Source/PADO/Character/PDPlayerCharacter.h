// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Interface/PDAimStateProvider.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PDPlayerCharacter.generated.h"

class APDWorldItemActor;
class UCameraComponent;
class USpringArmComponent;
class UPDRecoilComponent;

/** 조준 단계별 카메라 배치다. */
USTRUCT(BlueprintType)
struct PADO_API FPDAimCameraPose
{
	GENERATED_BODY()

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Aim",
		meta = (ClampMin = "0.0", Units = "cm"))
	float ArmLength = 400.0f;

	/** Spring Arm 기준 오프셋이다. Y가 오른쪽, Z가 위다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim")
	FVector SocketOffset = FVector::ZeroVector;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Aim",
		meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView = 90.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPDAimStateChangedSignature,
	EPDAimState,
	NewAimState);

/**
 * 플레이어가 조종하는 3인칭 캐릭터다.
 *
 * 카메라, 조준, 반동, 입력처럼 조종하는 사람에게만 의미가 있는 것을 가진다.
 * 다른 캐릭터와 공유하는 몸의 동작은 APDCharacterBase에 있다.
 * Ability System은 APDPlayerState가 소유하고, 이 캐릭터는 아바타로 연결된다.
 *
 * Input is owned and bound by APDPlayerController. The controller forwards
 * gameplay intent to the public functions on this class.
 */
UCLASS(Blueprintable)
class PADO_API APDPlayerCharacter
	: public APDCharacterBase
	, public IPDAimStateProvider
{
	GENERATED_BODY()

public:
	explicit APDPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaSeconds) override;

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

	/** 조준 입력을 누르고 있는 동안 견착으로 들어간다. 조준 중이었다면 견착으로 내려온다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	void StartShouldering();

	/** 조준 입력을 떼면 평상시 시점으로 돌아간다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	void StopShouldering();

	/** 조준 입력을 짧게 눌렀을 때 조준을 켜고 끈다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Input")
	void ToggleAiming();

	UFUNCTION(BlueprintCallable, Category = "PADO|Aim")
	void SetAimState(EPDAimState NewAimState);

	UFUNCTION(BlueprintPure, Category = "PADO|Aim")
	virtual EPDAimState GetAimState() const override;

	/** 조준 조건을 만족하는지 본다. 기본값에서는 아이템을 들고 있어야 한다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Aim")
	bool CanEnterAimState() const;

	/** 단계가 실제로 바뀔 때만 알린다. AnimBP가 여기에 붙는다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Aim")
	FPDAimStateChangedSignature OnAimStateChanged;

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

	UFUNCTION(BlueprintPure, Category = "PADO|Recoil")
	UPDRecoilComponent* GetRecoilComponent() const { return RecoilComponent; }

	UFUNCTION(BlueprintPure, Category = "PADO|Camera")
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintPure, Category = "PADO|Camera")
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	virtual void PawnClientRestart() override;

private:
	/**
	 * PlayerState의 ASC에 이 몸을 연결한다.
	 *
	 * 서버는 빙의 시점에 PlayerState가 있지만, 클라이언트는 PlayerState·
	 * Controller·Pawn이 제각각 도착한다. 도착할 수 있는 경계마다 부르고,
	 * 아직 없으면 다음 경계에서 다시 시도한다.
	 */
	void InitializePlayerAbilitySystem();

	const FPDAimCameraPose& GetAimCameraPose(EPDAimState State) const;
	void UpdateAimCamera(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Recoil", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDRecoilComponent> RecoilComponent;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Aim", meta = (AllowPrivateAccess = "true"))
	FPDAimCameraPose IdleCameraPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Aim", meta = (AllowPrivateAccess = "true"))
	FPDAimCameraPose ShoulderedCameraPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Aim", meta = (AllowPrivateAccess = "true"))
	FPDAimCameraPose AimingCameraPose;

	/** 클수록 단계 전환이 빠르다. 12면 0.2~0.3초 정도에 넘어간다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PADO|Aim",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float AimCameraInterpSpeed = 12.0f;

	/** 끄면 맨손으로도 견착·조준할 수 있다. 카메라만 확인할 때 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Aim", meta = (AllowPrivateAccess = "true"))
	bool bRequireHeldItemToAim = true;

	/**
	 * 마지막으로 알린 단계다. 소유 클라이언트는 입력으로, 서버는 이동 압축
	 * 플래그로 단계를 받기 때문에 양쪽 경로를 Tick에서 한 번에 비교한다.
	 */
	EPDAimState LastBroadcastAimState = EPDAimState::Idle;

	/** PlayerState 클래스 설정 오류는 경계마다 반복되므로 한 번만 알린다. */
	bool bWarnedUnexpectedPlayerState = false;
};
