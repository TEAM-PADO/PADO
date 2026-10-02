// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDPlayerController.h"

#include "PDPlayerCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "ChaosVehicleMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "PADO/Character/PDRespawnComponent.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"
#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"
#include "PADO/Vehicle/PDWheeledVehicle.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDPlayerController, Log, All);

APDPlayerController::APDPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 서브오브젝트 이름은 파생 Blueprint에 저장된 컴포넌트 값의 키다. 바꾸지 않는다.
	RespawnComponent = CreateDefaultSubobject<UPDRespawnComponent>(TEXT("Respawn"));
}

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

	// Drop과 조준은 선택 입력이다. 에셋을 지정하지 않으면 바인딩만 건너뛴다.
	if (DropAction)
	{
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &ThisClass::HandleDrop);
	}

	if (AimHoldAction)
	{
		// Hold 트리거는 임계 시간을 넘긴 뒤부터 Triggered를 반복한다. 진입은 멱등이다.
		EnhancedInputComponent->BindAction(AimHoldAction, ETriggerEvent::Triggered, this, &ThisClass::HandleAimHoldTriggered);
		EnhancedInputComponent->BindAction(AimHoldAction, ETriggerEvent::Completed, this, &ThisClass::HandleAimHoldCompleted);
		EnhancedInputComponent->BindAction(AimHoldAction, ETriggerEvent::Canceled, this, &ThisClass::HandleAimHoldCompleted);
	}

	if (AimToggleAction)
	{
		EnhancedInputComponent->BindAction(AimToggleAction, ETriggerEvent::Triggered, this, &ThisClass::HandleAimToggle);
	}

	// 운전 입력도 선택이다. 운전 매핑에 들어 있을 때만 발생한다.
	if (VehicleThrottleAction)
	{
		EnhancedInputComponent->BindAction(VehicleThrottleAction, ETriggerEvent::Triggered, this, &ThisClass::HandleVehicleThrottle);
		EnhancedInputComponent->BindAction(VehicleThrottleAction, ETriggerEvent::Completed, this, &ThisClass::HandleVehicleThrottleCompleted);
		EnhancedInputComponent->BindAction(VehicleThrottleAction, ETriggerEvent::Canceled, this, &ThisClass::HandleVehicleThrottleCompleted);
	}

	if (VehicleSteerAction)
	{
		EnhancedInputComponent->BindAction(VehicleSteerAction, ETriggerEvent::Triggered, this, &ThisClass::HandleVehicleSteer);
		EnhancedInputComponent->BindAction(VehicleSteerAction, ETriggerEvent::Completed, this, &ThisClass::HandleVehicleSteerCompleted);
		EnhancedInputComponent->BindAction(VehicleSteerAction, ETriggerEvent::Canceled, this, &ThisClass::HandleVehicleSteerCompleted);
	}

	if (VehicleBrakeAction)
	{
		EnhancedInputComponent->BindAction(VehicleBrakeAction, ETriggerEvent::Triggered, this, &ThisClass::HandleVehicleBrake);
		EnhancedInputComponent->BindAction(VehicleBrakeAction, ETriggerEvent::Completed, this, &ThisClass::HandleVehicleBrakeCompleted);
		EnhancedInputComponent->BindAction(VehicleBrakeAction, ETriggerEvent::Canceled, this, &ThisClass::HandleVehicleBrakeCompleted);
	}

	if (VehicleHandbrakeAction)
	{
		EnhancedInputComponent->BindAction(VehicleHandbrakeAction, ETriggerEvent::Started, this, &ThisClass::HandleVehicleHandbrakeStarted);
		EnhancedInputComponent->BindAction(VehicleHandbrakeAction, ETriggerEvent::Completed, this, &ThisClass::HandleVehicleHandbrakeCompleted);
		EnhancedInputComponent->BindAction(VehicleHandbrakeAction, ETriggerEvent::Canceled, this, &ThisClass::HandleVehicleHandbrakeCompleted);
	}

	if (VehicleExitAction)
	{
		EnhancedInputComponent->BindAction(VehicleExitAction, ETriggerEvent::Started, this, &ThisClass::HandleVehicleExit);
	}

	if (VehicleSeatSelectAction)
	{
		EnhancedInputComponent->BindAction(VehicleSeatSelectAction, ETriggerEvent::Started, this, &ThisClass::HandleVehicleSeatSelect);
	}

	if (VehicleNextSeatAction)
	{
		EnhancedInputComponent->BindAction(VehicleNextSeatAction, ETriggerEvent::Started, this, &ThisClass::HandleVehicleNextSeat);
	}
}

void APDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 엔진은 몸이 움직일 때만 카메라 위치를 서버에 보낸다. 탑승 중에는 몸의
	// 이동이 꺼져 있어 서버가 시점을 탄 자리로 기억한다. 서버는 이 시점으로 복제
	// 범위와 조준 방향을 정하므로 탑승 중에는 매 틱 보내게 한다. 실제 전송 빈도와
	// 변화 감지는 엔진이 한다.
	if (ViewedVehicle.IsValid() &&
		PlayerCameraManager &&
		PlayerCameraManager->bUseClientSideCameraUpdates)
	{
		PlayerCameraManager->bShouldSendClientSideCameraUpdate = true;
	}
}

void APDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveSeatMappingContext();
	RemoveDefaultMappingContext();
	Super::EndPlay(EndPlayReason);
}

void APDPlayerController::BeginVehicleControl(APDWheeledVehicle* Vehicle)
{
	if (!IsLocalController() || !Vehicle)
	{
		return;
	}

	ControlledVehicle = Vehicle;
}

void APDPlayerController::EndVehicleControl(APDWheeledVehicle* Vehicle)
{
	if (!IsLocalController() || !Vehicle || ControlledVehicle.Get() != Vehicle)
	{
		return;
	}

	// 서버가 입력을 넘겨받기 전까지 마지막 입력이 남지 않게 비운다.
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetThrottleInput(0.0f);
		Movement->SetSteeringInput(0.0f);
		Movement->SetBrakeInput(0.0f);
		Movement->SetHandbrakeInput(false);
	}

	ControlledVehicle.Reset();
}

APDWheeledVehicle* APDPlayerController::GetControlledVehicle() const
{
	return ControlledVehicle.Get();
}

void APDPlayerController::BeginVehicleView(const UPDVehicleSeatComponent& Seat)
{
	AActor* Vehicle = Seat.GetOwner();
	if (!IsLocalController() || !Vehicle)
	{
		return;
	}

	// 같은 탈것 안에서 좌석을 옮겼다. 시점은 그대로 둔다.
	if (ViewedVehicle.Get() == Vehicle)
	{
		RefreshSeatMappingContext(Seat);
		return;
	}

	// 진행 중이던 손 행동은 탑승 상태가 몸에 적용될 때 이미 끊었다
	// (APDCharacterBase::InterruptHandActions).
	ViewedVehicle = Vehicle;
	SetViewTargetWithBlend(Vehicle, VehicleCameraBlendTime);
	AddSeatMappingContext(Seat);
}

void APDPlayerController::EndVehicleView()
{
	// 탈것이 먼저 사라졌어도 시점과 입력은 되돌린다. 약한 참조가 끊긴 것과
	// 처음부터 없던 것을 구분한다.
	if (!IsLocalController() || ViewedVehicle.IsExplicitlyNull())
	{
		return;
	}

	ViewedVehicle.Reset();
	if (RemoveSeatMappingContext())
	{
		AddDefaultMappingContext();
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		SetViewTargetWithBlend(ControlledPawn, VehicleCameraBlendTime);
	}
}

void APDPlayerController::AddSeatMappingContext(const UPDVehicleSeatComponent& Seat)
{
	if (AddedSeatMappingContext)
	{
		return;
	}

	const bool bDriverSeat = Seat.IsDriverSeat();
	UInputMappingContext* SeatMappingContext = GetSeatMappingContext(Seat);
	if (!SeatMappingContext)
	{
		// 매핑을 다 빼 버리면 내릴 입력도 없어진다. 캐릭터 입력을 그대로 둔다.
		bool& bWarned = bDriverSeat
			? bWarnedMissingVehicleDriverMapping
			: bWarnedMissingVehiclePassengerMapping;
		if (!bWarned)
		{
			UE_LOG(
				LogPDPlayerController,
				Warning,
				TEXT("%s: %s가 없어 캐릭터 입력을 그대로 둔다. %s"),
				*GetNameSafe(this),
				bDriverSeat ? TEXT("VehicleDriverMappingContext") : TEXT("VehiclePassengerMappingContext"),
				bDriverSeat
					? TEXT("운전 입력이 동작하지 않는다.")
					: TEXT("동승석에서도 손에 든 것을 쓸 수 있다."));
			bWarned = true;
		}
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	if (Subsystem)
	{
		RemoveDefaultMappingContext();
		Subsystem->AddMappingContext(SeatMappingContext, DefaultMappingPriority);
		AddedSeatMappingContext = SeatMappingContext;
	}
}

bool APDPlayerController::RemoveSeatMappingContext()
{
	if (!AddedSeatMappingContext)
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	if (Subsystem)
	{
		Subsystem->RemoveMappingContext(AddedSeatMappingContext);
	}

	AddedSeatMappingContext = nullptr;
	return true;
}

void APDPlayerController::RefreshSeatMappingContext(const UPDVehicleSeatComponent& Seat)
{
	if (AddedSeatMappingContext == GetSeatMappingContext(Seat))
	{
		return;
	}

	// 바꾸는 동안 누르고 있던 키는 뗄 때까지 무시되므로 좌석 키가 반복되지 않는다.
	if (RemoveSeatMappingContext())
	{
		AddDefaultMappingContext();
	}
	AddSeatMappingContext(Seat);
}

UInputMappingContext* APDPlayerController::GetSeatMappingContext(
	const UPDVehicleSeatComponent& Seat) const
{
	return Seat.IsDriverSeat()
		? VehicleDriverMappingContext
		: VehiclePassengerMappingContext;
}

UPDVehicleOccupantComponent* APDPlayerController::GetVehicleOccupant() const
{
	const APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter();
	return ControlledCharacter ? ControlledCharacter->GetVehicleOccupantComponent() : nullptr;
}

UChaosVehicleMovementComponent* APDPlayerController::GetControlledVehicleMovement() const
{
	const APDWheeledVehicle* Vehicle = ControlledVehicle.Get();
	return Vehicle ? Vehicle->GetVehicleMovementComponent() : nullptr;
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

void APDPlayerController::HandleAimHoldTriggered()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StartShouldering();
	}
}

void APDPlayerController::HandleAimHoldCompleted()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->StopShouldering();
	}
}

void APDPlayerController::HandleAimToggle()
{
	if (APDPlayerCharacter* ControlledCharacter = GetPDPlayerCharacter())
	{
		ControlledCharacter->ToggleAiming();
	}
}

void APDPlayerController::HandleVehicleThrottle(const FInputActionValue& Value)
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetThrottleInput(Value.Get<float>());
	}
}

void APDPlayerController::HandleVehicleThrottleCompleted()
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetThrottleInput(0.0f);
	}
}

void APDPlayerController::HandleVehicleSteer(const FInputActionValue& Value)
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetSteeringInput(Value.Get<float>());
	}
}

void APDPlayerController::HandleVehicleSteerCompleted()
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetSteeringInput(0.0f);
	}
}

void APDPlayerController::HandleVehicleBrake(const FInputActionValue& Value)
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetBrakeInput(Value.Get<float>());
	}
}

void APDPlayerController::HandleVehicleBrakeCompleted()
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetBrakeInput(0.0f);
	}
}

void APDPlayerController::HandleVehicleHandbrakeStarted()
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetHandbrakeInput(true);
	}
}

void APDPlayerController::HandleVehicleHandbrakeCompleted()
{
	if (UChaosVehicleMovementComponent* Movement = GetControlledVehicleMovement())
	{
		Movement->SetHandbrakeInput(false);
	}
}

void APDPlayerController::HandleVehicleExit()
{
	if (UPDVehicleOccupantComponent* Occupant = GetVehicleOccupant())
	{
		Occupant->RequestExit();
	}
}

void APDPlayerController::HandleVehicleSeatSelect(const FInputActionValue& Value)
{
	if (UPDVehicleOccupantComponent* Occupant = GetVehicleOccupant())
	{
		Occupant->RequestSwitchSeat(FMath::RoundToInt(Value.Get<float>()));
	}
}

void APDPlayerController::HandleVehicleNextSeat()
{
	if (UPDVehicleOccupantComponent* Occupant = GetVehicleOccupant())
	{
		Occupant->RequestSwitchToNextSeat();
	}
}
