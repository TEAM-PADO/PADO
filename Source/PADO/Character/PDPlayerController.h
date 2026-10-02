// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PDPlayerController.generated.h"

class APDPlayerCharacter;
class APDWheeledVehicle;
class UChaosVehicleMovementComponent;
class UInputAction;
class UInputMappingContext;
class UPDRespawnComponent;
class UPDVehicleOccupantComponent;
class UPDVehicleSeatComponent;
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
	explicit APDPlayerController(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "PADO|Respawn")
	UPDRespawnComponent* GetRespawnComponent() const { return RespawnComponent; }

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

	/**
	 * 이 머신의 플레이어가 차량 조종을 시작한다. 조종자가 바뀌면 차량이 부른다.
	 * 운전 입력을 보낼 차량을 기억한다. 입력 매핑과 카메라는 좌석이 바꾼다(BeginVehicleView).
	 */
	void BeginVehicleControl(APDWheeledVehicle* Vehicle);

	/** 조종하던 차량에서 손을 뗀다. 남아 있는 운전 입력을 비운다. */
	void EndVehicleControl(APDWheeledVehicle* Vehicle);

	UFUNCTION(BlueprintPure, Category = "PADO|Vehicle")
	APDWheeledVehicle* GetControlledVehicle() const;

	/**
	 * 이 머신의 플레이어가 탈것 좌석에 앉았다. 운전석이든 동승석이든 탈것
	 * 카메라로 넘어간다. 탈것 카메라는 머신마다 따로 돌므로 탑승자마다 시점이 따로 돈다.
	 *
	 * 탑승 중에는 손에 든 것을 쓰지 않는다. 캐릭터 입력(IMC_Player)을 빼고 좌석
	 * 역할에 맞는 매핑만 넣는다. 같은 탈것 안에서 좌석을 옮기면 시점은 그대로 두고
	 * 매핑만 새 좌석 역할에 맞춘다.
	 */
	void BeginVehicleView(const UPDVehicleSeatComponent& Seat);

	/** 탈것에서 내렸거나 탈것이 사라졌다. 캐릭터 카메라와 캐릭터 입력으로 돌아온다. */
	void EndVehicleView();

	/** 이 머신의 플레이어가 앉아서 보고 있는 탈것이다. 탈것은 이 값으로 카메라를 돌릴 컨트롤러를 찾는다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Vehicle")
	AActor* GetViewedVehicle() const { return ViewedVehicle.Get(); }

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Priority used when registering the player mapping context with Enhanced Input. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Input")
	int32 DefaultMappingPriority = 0;

	/** 탈것 카메라와 캐릭터 카메라를 오갈 때의 전환 시간이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (ClampMin = "0.0", Units = "s"))
	float VehicleCameraBlendTime = 0.3f;

private:
	void AddDefaultMappingContext();
	void RemoveDefaultMappingContext();
	/** 좌석 역할에 맞는 매핑으로 캐릭터 입력을 대신한다. 매핑이 없으면 캐릭터 입력을 그대로 둔다. */
	void AddSeatMappingContext(const UPDVehicleSeatComponent& Seat);

	/** 넣어 둔 좌석 매핑을 뺀다. 뺀 것이 있으면 true다. */
	bool RemoveSeatMappingContext();

	/** 넣어 둔 좌석 매핑이 이 좌석 역할의 것이 아니면 바꾼다. */
	void RefreshSeatMappingContext(const UPDVehicleSeatComponent& Seat);

	UInputMappingContext* GetSeatMappingContext(const UPDVehicleSeatComponent& Seat) const;
	UPDVehicleOccupantComponent* GetVehicleOccupant() const;
	UChaosVehicleMovementComponent* GetControlledVehicleMovement() const;

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
	void HandleVehicleThrottle(const FInputActionValue& Value);
	void HandleVehicleThrottleCompleted();
	void HandleVehicleSteer(const FInputActionValue& Value);
	void HandleVehicleSteerCompleted();
	void HandleVehicleBrake(const FInputActionValue& Value);
	void HandleVehicleBrakeCompleted();
	void HandleVehicleHandbrakeStarted();
	void HandleVehicleHandbrakeCompleted();
	void HandleVehicleExit();
	void HandleVehicleSeatSelect(const FInputActionValue& Value);
	void HandleVehicleNextSeat();

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

	/**
	 * 운전석에서 캐릭터 입력 대신 쓰는 매핑이다. 탑승 중에는 손에 든 것을 쓰지
	 * 않으므로 IMC_Player를 빼고 이것만 쓴다. 시점 회전과 하차도 여기에 매핑한다.
	 * 비워 두면 운전석에 앉아도 캐릭터 입력을 그대로 두고 경고를 남긴다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> VehicleDriverMappingContext;

	/**
	 * 동승석에서 캐릭터 입력 대신 쓰는 매핑이다. 시점 회전과 하차만 넣는다.
	 * 비워 두면 동승석에 앉아도 캐릭터 입력을 그대로 두고 경고를 남긴다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> VehiclePassengerMappingContext;

	/** Axis1D. 앞으로 가는 힘이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleThrottleAction;

	/** Axis1D. 오른쪽이 양수다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleSteerAction;

	/** Axis1D. 멈춘 상태에서 계속 누르면 후진한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleBrakeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleHandbrakeAction;

	/** 탈것에서 내린다. 좌석 매핑에는 상호작용 입력이 없으므로 따로 둔다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleExitAction;

	/**
	 * Axis1D. 값이 옮겨 갈 좌석 번호다. 키마다 Scalar 수정자로 좌석 번호를 넣는다
	 * (Ctrl+1이면 1, Ctrl+2면 2). 차 있거나 없는 좌석이면 서버가 거부한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleSeatSelectAction;

	/** 좌석 순서대로 다음 빈 좌석으로 옮긴다. 게임패드처럼 번호 키가 없는 입력용이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PADO|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> VehicleNextSeatAction;

	/** 죽으면 새 몸으로 다시 시작시킨다. 서버에서만 동작한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PADO|Respawn", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDRespawnComponent> RespawnComponent;

	/** 이 머신의 플레이어가 조종 중인 차량이다. */
	TWeakObjectPtr<APDWheeledVehicle> ControlledVehicle;

	/** 이 머신의 플레이어가 앉아서 보고 있는 탈것이다. 조종 여부와 상관없다. */
	TWeakObjectPtr<AActor> ViewedVehicle;

	bool bDefaultMappingContextAdded = false;

	/** 지금 넣어 둔 좌석 매핑이다. 내릴 때 이것을 뺀다. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> AddedSeatMappingContext;

	/** 좌석 매핑 누락은 탈 때마다 반복되므로 좌석 역할마다 한 번만 알린다. */
	bool bWarnedMissingVehicleDriverMapping = false;
	bool bWarnedMissingVehiclePassengerMapping = false;
};
