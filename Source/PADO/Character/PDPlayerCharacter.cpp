// Copyright Epic Games, Inc. All Rights Reserved.

#include "PDPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/OverlapResult.h"
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
}

APDPlayerCharacter::APDPlayerCharacter()
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

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = PDCharacterDefaults::CameraBoomLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AbilitySystemComponent =
		CreateDefaultSubobject<UPDAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	HeldItemComponent =
		CreateDefaultSubobject<UPDHeldItemComponent>(TEXT("HeldItem"));
	KnockbackComponent =
		CreateDefaultSubobject<UPDKnockbackComponent>(TEXT("Knockback"));
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
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
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
	// Intentionally empty. Sprinting requires an authoritative, predicted movement implementation.
}

void APDPlayerCharacter::StopSprinting_Implementation()
{
	// Intentionally empty. Sprinting requires an authoritative, predicted movement implementation.
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
