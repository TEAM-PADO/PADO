// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/OverlapResult.h"
#include "PADO/AbilitySystem/Attribute/PDMovementAttributeSet.h"
#include "PADO/Character/PDCharacterMovementComponent.h"
#include "PADO/Character/PDRecoilComponent.h"
#include "Engine/World.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Component/PDKnockbackComponent.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

namespace PDCharacterDefaults
{
	constexpr float CapsuleRadius = 42.0f;
	constexpr float CapsuleHalfHeight = 96.0f;
	constexpr float RotationRateYaw = 500.0f;
	constexpr float JumpZVelocity = 500.0f;
	constexpr float AirControl = 0.35f;
	constexpr float MaxWalkSpeed = 500.0f;
	constexpr float MinAnalogWalkSpeed = 20.0f;
	constexpr float BrakingDecelerationWalking = 2000.0f;
	constexpr float BrakingDecelerationFalling = 1500.0f;
	constexpr float CameraBoomLength = 400.0f;

	constexpr float IdleFieldOfView = 90.0f;
	constexpr float ShoulderedArmLength = 160.0f;
	constexpr float ShoulderedFieldOfView = 85.0f;
	constexpr float AimingArmLength = 110.0f;
	constexpr float AimingFieldOfView = 60.0f;
	const FVector ShoulderedSocketOffset(0.0f, 60.0f, 50.0f);
	const FVector AimingSocketOffset(0.0f, 45.0f, 45.0f);

	/** 보간 종료 판정 허용 오차다. */
	constexpr float CameraSettleTolerance = 0.1f;
}

APDPlayerCharacter::APDPlayerCharacter(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPDCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	GetCapsuleComponent()->InitCapsuleSize(PDCharacterDefaults::CapsuleRadius, PDCharacterDefaults::CapsuleHalfHeight);

	// 캐릭터는 이동 방향이 아니라 카메라 회전을 따른다. 몸 방향과 조준 방향이
	// 항상 일치해야 옆이나 뒤로 이동하면서 쏠 때 연출과 탄착점이 어긋나지 않는다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->RotationRate = FRotator(0.0f, PDCharacterDefaults::RotationRateYaw, 0.0f);
	MovementComponent->JumpZVelocity = PDCharacterDefaults::JumpZVelocity;
	MovementComponent->AirControl = PDCharacterDefaults::AirControl;
	MovementComponent->MaxWalkSpeed = PDCharacterDefaults::MaxWalkSpeed;
	MovementComponent->MinAnalogWalkSpeed = PDCharacterDefaults::MinAnalogWalkSpeed;
	MovementComponent->BrakingDecelerationWalking = PDCharacterDefaults::BrakingDecelerationWalking;
	MovementComponent->BrakingDecelerationFalling = PDCharacterDefaults::BrakingDecelerationFalling;

	IdleCameraPose.ArmLength = PDCharacterDefaults::CameraBoomLength;
	IdleCameraPose.SocketOffset = FVector::ZeroVector;
	IdleCameraPose.FieldOfView = PDCharacterDefaults::IdleFieldOfView;

	ShoulderedCameraPose.ArmLength = PDCharacterDefaults::ShoulderedArmLength;
	ShoulderedCameraPose.SocketOffset = PDCharacterDefaults::ShoulderedSocketOffset;
	ShoulderedCameraPose.FieldOfView = PDCharacterDefaults::ShoulderedFieldOfView;

	AimingCameraPose.ArmLength = PDCharacterDefaults::AimingArmLength;
	AimingCameraPose.SocketOffset = PDCharacterDefaults::AimingSocketOffset;
	AimingCameraPose.FieldOfView = PDCharacterDefaults::AimingFieldOfView;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = IdleCameraPose.ArmLength;
	CameraBoom->SocketOffset = IdleCameraPose.SocketOffset;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(IdleCameraPose.FieldOfView);

	AbilitySystemComponent =
		CreateDefaultSubobject<UPDAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	MovementAttributes =
		CreateDefaultSubobject<UPDMovementAttributeSet>(TEXT("MovementAttributes"));

	HeldItemComponent =
		CreateDefaultSubobject<UPDHeldItemComponent>(TEXT("HeldItem"));
	KnockbackComponent =
		CreateDefaultSubobject<UPDKnockbackComponent>(TEXT("Knockback"));
	RecoilComponent =
		CreateDefaultSubobject<UPDRecoilComponent>(TEXT("Recoil"));
}

UAbilitySystemComponent* APDPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeAbilityActorInfo();
}

void APDPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 조준 중 무기를 잃으면 단계를 유지할 근거가 없다. 드롭·파괴 모두 여기로 모인다.
	if (GetAimState() != EPDAimState::Idle && !CanEnterAimState())
	{
		SetAimState(EPDAimState::Idle);
	}

	// 소유 클라이언트는 입력으로, 서버는 이동 압축 플래그로 단계를 받는다.
	// 두 경로를 한곳에서 비교해 실제로 바뀐 순간에만 알린다.
	const EPDAimState CurrentAimState = GetAimState();
	if (LastBroadcastAimState != CurrentAimState)
	{
		LastBroadcastAimState = CurrentAimState;
		OnAimStateChanged.Broadcast(CurrentAimState);
	}

	UpdateAimCamera(DeltaSeconds);
}

void APDPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilityActorInfo();
}

void APDPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilityActorInfo();
}

void APDPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	InitializeAbilityActorInfo();
}

void APDPlayerCharacter::InitializeAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// ASC는 InitializeComponent에서 소유자의 AttributeSet을 자동 수집하지만,
	// 그건 Outer가 같은 Actor일 때만이고 실행 시점도 액터 초기화에 묶여 있다.
	// 나중에 ASC를 PlayerState로 옮기면 자동 수집이 닿지 않으므로 직접 등록한다.
	// AddSpawnedAttribute는 AddUnique라 여러 번 불려도 안전하다.
	if (MovementAttributes)
	{
		AbilitySystemComponent->AddSpawnedAttribute(MovementAttributes);
	}

	// 이 함수는 BeginPlay/Possess/OnRep/Restart 경계마다 불린다. 중복 등록을 막는다.
	if (!bMoveSpeedDelegateBound)
	{
		AbilitySystemComponent
			->GetGameplayAttributeValueChangeDelegate(
				UPDMovementAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &APDPlayerCharacter::HandleMoveSpeedChanged);
		bMoveSpeedDelegateBound = true;
	}

	PushMoveSpeedToMovement();
}

UPDCharacterMovementComponent* APDPlayerCharacter::GetPDCharacterMovement() const
{
	return Cast<UPDCharacterMovementComponent>(GetCharacterMovement());
}

void APDPlayerCharacter::PushMoveSpeedToMovement()
{
	UPDCharacterMovementComponent* Movement = GetPDCharacterMovement();
	if (Movement && MovementAttributes)
	{
		Movement->SetAttributeMoveSpeed(MovementAttributes->GetMoveSpeed());
	}
}

void APDPlayerCharacter::HandleMoveSpeedChanged(
	const FOnAttributeChangeData& ChangeData)
{
	if (UPDCharacterMovementComponent* Movement = GetPDCharacterMovement())
	{
		Movement->SetAttributeMoveSpeed(ChangeData.NewValue);
	}
}

void APDPlayerCharacter::Move(const FVector2D& MovementInput)
{
	if (!Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

void APDPlayerCharacter::Look(const FVector2D& LookInput)
{
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);

	// 유저가 반동을 눌러 상쇄한 만큼은 복원하지 않아야 한다.
	if (RecoilComponent)
	{
		RecoilComponent->NotifyLookInput(LookInput);
	}
}

void APDPlayerCharacter::StartJump()
{
	Jump();
}

void APDPlayerCharacter::StopJump()
{
	StopJumping();
}

void APDPlayerCharacter::StartSprinting_Implementation()
{
	// 의도만 세운다. 실제 속도 판정은 무브먼트가 조준 단계와 함께 결정하고,
	// 압축 플래그로 서버에 전달돼 같은 값으로 재생된다.
	if (UPDCharacterMovementComponent* Movement = GetPDCharacterMovement())
	{
		Movement->SetWantsToSprint(true);
	}
}

void APDPlayerCharacter::StopSprinting_Implementation()
{
	if (UPDCharacterMovementComponent* Movement = GetPDCharacterMovement())
	{
		Movement->SetWantsToSprint(false);
	}
}

void APDPlayerCharacter::Interact_Implementation()
{
	// 한 번에 하나만 들 수 있다. 교체하려면 먼저 내려놓는다.
	if (!HeldItemComponent || HeldItemComponent->HasHeldItem())
	{
		return;
	}

	if (APDWorldItemActor* Target = FindInteractTarget())
	{
		HeldItemComponent->RequestPickUp(Target);
	}
}

void APDPlayerCharacter::DropHeldItem()
{
	if (HeldItemComponent)
	{
		HeldItemComponent->TryDropHeldItem();
	}
}

APDWorldItemActor* APDPlayerCharacter::FindInteractTarget() const
{
	const UWorld* World = GetWorld();
	if (!World || !FollowCamera || !HeldItemComponent)
	{
		return nullptr;
	}

	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceEnd =
		TraceStart + FollowCamera->GetForwardVector() * InteractTraceDistance;

	FCollisionQueryParams QueryParams(TEXT("PDInteractTrace"), false, this);
	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(
		Hits,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(InteractTraceRadius),
		QueryParams);

	// Sweep 결과는 시작점에서 가까운 순서다. 카메라가 캐릭터 뒤에 있으므로
	// 먼저 걸리는 것이 곧 시선상 가장 앞의 후보다. CanPickUpItem이 캐릭터
	// 기준 거리와 아이템 상태를 함께 거른다.
	for (const FHitResult& Hit : Hits)
	{
		APDWorldItemActor* Item = Cast<APDWorldItemActor>(Hit.GetActor());
		if (IsValid(Item) && HeldItemComponent->CanPickUpItem(Item))
		{
			return Item;
		}
	}

	// 시선 Sweep만으로는 바닥에 놓인 아이템을 집을 수 없다. 카메라가 캐릭터
	// 중심 높이에서 수평으로 나가는 동안 아이템은 그보다 한참 아래에 있어서
	// 스쳐 지나간다. 드롭한 아이템을 다시 줍는 경우가 특히 그렇다.
	// 그래서 시선에 걸린 것이 없으면 줍기 반경 안의 가장 가까운 후보를 고른다.
	return FindNearestPickupCandidate();
}

APDWorldItemActor* APDPlayerCharacter::FindNearestPickupCandidate() const
{
	const UWorld* World = GetWorld();
	if (!World || !HeldItemComponent)
	{
		return nullptr;
	}

	const float SearchRadius = HeldItemComponent->GetMaxPickupDistance();
	if (SearchRadius <= 0.0f)
	{
		return nullptr;
	}

	FCollisionQueryParams QueryParams(TEXT("PDPickupOverlap"), false, this);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(SearchRadius),
		QueryParams);

	APDWorldItemActor* Nearest = nullptr;
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		APDWorldItemActor* Item = Cast<APDWorldItemActor>(Overlap.GetActor());
		if (!IsValid(Item) || !HeldItemComponent->CanPickUpItem(Item))
		{
			continue;
		}

		const double DistanceSquared =
			FVector::DistSquared(GetActorLocation(), Item->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Item;
		}
	}

	return Nearest;
}

void APDPlayerCharacter::Attack_Implementation()
{
	// Held Item이 없을 때 사용할 비무장 공격 등의 확장 지점이다.
}

bool APDPlayerCharacter::StartAttacking()
{
	if (HeldItemComponent && HeldItemComponent->HasHeldItem())
	{
		// 반동은 Held Item이 발사할 때마다 보내는 신호를 구독한다.
		// 여기서 따로 치면 게이트가 두 벌이 되어 반드시 어긋난다.
		return HeldItemComponent->PressHeldItemUse();
	}

	Attack();
	return false;
}

void APDPlayerCharacter::StopAttacking()
{
	if (HeldItemComponent)
	{
		HeldItemComponent->ReleaseHeldItemUse();
	}
}

bool APDPlayerCharacter::ReloadHeldItem()
{
	return HeldItemComponent && HeldItemComponent->TryReloadHeldItem();
}

void APDPlayerCharacter::StartShouldering()
{
	// 조준 중에 다시 누르고 있으면 견착으로 내려온다. 떼면 Idle로 간다.
	SetAimState(EPDAimState::Shouldered);
}

void APDPlayerCharacter::StopShouldering()
{
	// 토글로 켠 조준은 입력을 떼도 유지한다.
	if (GetAimState() == EPDAimState::Shouldered)
	{
		SetAimState(EPDAimState::Idle);
	}
}

void APDPlayerCharacter::ToggleAiming()
{
	SetAimState(
		GetAimState() == EPDAimState::Aiming
			? EPDAimState::Idle
			: EPDAimState::Aiming);
}

void APDPlayerCharacter::SetAimState(EPDAimState NewAimState)
{
	UPDCharacterMovementComponent* Movement = GetPDCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (NewAimState != EPDAimState::Idle && !CanEnterAimState())
	{
		NewAimState = EPDAimState::Idle;
	}

	// 단계는 무브먼트가 소유한다. 이동 압축 플래그로 서버에 전달되고
	// 서버가 같은 값으로 이동을 재생하므로 속도가 어긋나지 않는다.
	Movement->SetAimState(NewAimState);
}

EPDAimState APDPlayerCharacter::GetAimState() const
{
	const UPDCharacterMovementComponent* Movement = GetPDCharacterMovement();
	return Movement ? Movement->GetAimState() : EPDAimState::Idle;
}

bool APDPlayerCharacter::CanEnterAimState() const
{
	return !bRequireHeldItemToAim ||
		(HeldItemComponent && HeldItemComponent->HasHeldItem());
}

const FPDAimCameraPose& APDPlayerCharacter::GetAimCameraPose(
	EPDAimState State) const
{
	switch (State)
	{
	case EPDAimState::Shouldered:
		return ShoulderedCameraPose;

	case EPDAimState::Aiming:
		return AimingCameraPose;

	case EPDAimState::Idle:
	default:
		return IdleCameraPose;
	}
}

void APDPlayerCharacter::UpdateAimCamera(float DeltaSeconds)
{
	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	const FPDAimCameraPose& TargetPose = GetAimCameraPose(GetAimState());

	// 목표에 도달했으면 매 프레임 계산하지 않는다.
	if (FMath::IsNearlyEqual(
			CameraBoom->TargetArmLength,
			TargetPose.ArmLength,
			PDCharacterDefaults::CameraSettleTolerance) &&
		CameraBoom->SocketOffset.Equals(
			TargetPose.SocketOffset,
			PDCharacterDefaults::CameraSettleTolerance) &&
		FMath::IsNearlyEqual(
			FollowCamera->FieldOfView,
			TargetPose.FieldOfView,
			PDCharacterDefaults::CameraSettleTolerance))
	{
		return;
	}

	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		TargetPose.ArmLength,
		DeltaSeconds,
		AimCameraInterpSpeed);
	CameraBoom->SocketOffset = FMath::VInterpTo(
		CameraBoom->SocketOffset,
		TargetPose.SocketOffset,
		DeltaSeconds,
		AimCameraInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(
		FollowCamera->FieldOfView,
		TargetPose.FieldOfView,
		DeltaSeconds,
		AimCameraInterpSpeed));
}
