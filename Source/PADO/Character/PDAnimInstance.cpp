#include "PADO/Character/PDAnimInstance.h"

#include "PADO/Character/PDCharacterMovementComponent.h"
#include "PADO/Character/PDPlayerCharacter.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/PDWorldItemActor.h"

void UPDAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<APDPlayerCharacter>(TryGetPawnOwner());
	OwningMovement = OwningCharacter
		? OwningCharacter->GetPDCharacterMovement()
		: nullptr;
}

void UPDAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 리스폰이나 빙의 변경으로 소유 Pawn이 바뀔 수 있다.
	if (!OwningCharacter || OwningCharacter != TryGetPawnOwner())
	{
		OwningCharacter = Cast<APDPlayerCharacter>(TryGetPawnOwner());
		OwningMovement = OwningCharacter
			? OwningCharacter->GetPDCharacterMovement()
			: nullptr;
	}

	if (!OwningCharacter)
	{
		return;
	}

	RefreshLocomotion();
	RefreshAim();
	RefreshHeldItem();
}

void UPDAnimInstance::RefreshLocomotion()
{
	Velocity = OwningCharacter->GetVelocity();
	GroundSpeed = Velocity.Size2D();

	// 속도가 거의 0이면 방향을 계산할 수 없다. 마지막 값을 들고 있으면
	// 멈추는 순간 다리가 튀므로 정면으로 되돌린다.
	Direction = GroundSpeed > UE_KINDA_SMALL_NUMBER
		? FRotator::NormalizeAxis(
			Velocity.Rotation().Yaw - OwningCharacter->GetActorRotation().Yaw)
		: 0.0f;

	if (!OwningMovement)
	{
		bShouldMove = false;
		bIsFalling = false;
		bIsSprinting = false;
		return;
	}

	// 가속 입력이 없으면 미끄러지는 중이므로 걷기 애니메이션을 틀지 않는다.
	bShouldMove = GroundSpeed > MoveThresholdSpeed &&
		!OwningMovement->GetCurrentAcceleration().IsNearlyZero();
	bIsFalling = OwningMovement->IsFalling();
	bIsSprinting = OwningMovement->IsSprinting();
}

void UPDAnimInstance::RefreshAim()
{
	AimState = OwningCharacter->GetAimState();

	const FRotator AimDelta = (OwningCharacter->GetBaseAimRotation() -
		OwningCharacter->GetActorRotation()).GetNormalized();
	AimPitch = AimDelta.Pitch;
	AimYaw = AimDelta.Yaw;
}

void UPDAnimInstance::RefreshHeldItem()
{
	bHasHeldItem = false;
	HeldPose = EPDHeldPose::Default;
	HeldItemId = FGameplayTag();

	const UPDHeldItemComponent* HeldItems =
		OwningCharacter->GetHeldItemComponent();
	const APDWorldItemActor* HeldItem = HeldItems
		? HeldItems->GetHeldItem()
		: nullptr;
	const UPDItemDefinition* Definition = HeldItem
		? HeldItem->GetItemDefinition()
		: nullptr;
	if (!Definition)
	{
		return;
	}

	bHasHeldItem = true;
	HeldPose = Definition->Presentation.HeldPose;
	HeldItemId = Definition->ItemId;
}
