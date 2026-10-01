#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Character/PDPlayerController.h"
#include "PADO/Vehicle/Component/PDVehicleOccupancyComponent.h"
#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"

UPDVehicleOccupantComponent::UPDVehicleOccupantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

AActor* UPDVehicleOccupantComponent::GetCurrentVehicle() const
{
	return State.Seat ? State.Seat->GetOwner() : nullptr;
}

bool UPDVehicleOccupantComponent::RequestExit()
{
	AActor* Owner = GetOwner();
	if (!Owner || !IsSeated())
	{
		return false;
	}

	if (Owner->HasAuthority())
	{
		UPDVehicleOccupancyComponent* Occupancy = ResolveOccupancy();
		return Occupancy && Occupancy->TryExit(Cast<APDCharacterBase>(Owner));
	}

	ServerRequestExit();
	return true;
}

void UPDVehicleOccupantComponent::ServerRequestExit_Implementation()
{
	RequestExit();
}

void UPDVehicleOccupantComponent::EnterSeat(UPDVehicleSeatComponent* Seat)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Seat)
	{
		return;
	}

	State.Seat = Seat;
	ApplyState();
	Owner->ForceNetUpdate();
}

void UPDVehicleOccupantComponent::ExitSeat(const FVector& ExitLocation)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	State.Seat = nullptr;
	State.ExitLocation = ExitLocation;
	ApplyState();
	Owner->ForceNetUpdate();
}

void UPDVehicleOccupantComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 몸이 탄 채로 사라지면(접속 종료 등) 좌석을 비운다. 조종석이면 조종 권한도 거둔다.
	if (GetOwner() && GetOwner()->HasAuthority() && IsSeated())
	{
		if (UPDVehicleOccupancyComponent* Occupancy = ResolveOccupancy())
		{
			Occupancy->ReleaseOccupant(Cast<APDCharacterBase>(GetOwner()));
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UPDVehicleOccupantComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDVehicleOccupantComponent, State);
}

void UPDVehicleOccupantComponent::OnRep_State()
{
	ApplyState();
}

UPDVehicleOccupancyComponent* UPDVehicleOccupantComponent::ResolveOccupancy() const
{
	const AActor* Vehicle = GetCurrentVehicle();
	return Vehicle ? Vehicle->FindComponentByClass<UPDVehicleOccupancyComponent>() : nullptr;
}

ACharacter* UPDVehicleOccupantComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

APDPlayerController* UPDVehicleOccupantComponent::GetLocalPlayerController() const
{
	const ACharacter* Character = GetOwnerCharacter();
	return Character && Character->IsLocallyControlled()
		? Cast<APDPlayerController>(Character->GetController())
		: nullptr;
}

void UPDVehicleOccupantComponent::ApplyState()
{
	if (State.Seat)
	{
		ApplySeated(*State.Seat);
	}
	else if (bSeatedApplied)
	{
		ApplyUnseated();
	}
}

void UPDVehicleOccupantComponent::ApplySeated(UPDVehicleSeatComponent& Seat)
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	if (!bSeatedApplied)
	{
		bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
	}

	// 좌석 안에서 몸이 시점을 따라 돌지 않게 한다. 시점은 그대로 돌릴 수 있다.
	Character->bUseControllerRotationYaw = false;
	Character->SetActorEnableCollision(false);
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();

		// 몸은 좌석을 따라 움직인다. 이동 예측을 멈춰 좌석 위치를 두고 서버와
		// 보정을 주고받지 않게 한다.
		Movement->SetComponentTickEnabled(false);
	}

	Character->AttachToComponent(
		&Seat,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	bSeatedApplied = true;

	// 시점과 입력은 이 머신에서 조종하는 탑승자만 바꾼다. 다른 탑승자는 그 머신이 바꾼다.
	if (APDPlayerController* PlayerController = GetLocalPlayerController())
	{
		PlayerController->BeginVehicleView(Seat);
	}
}

void UPDVehicleOccupantComponent::ApplyUnseated()
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 좌석의 기울기는 버리고 바라보던 방향만 남긴다.
	const FRotator ExitRotation(0.0f, Character->GetActorRotation().Yaw, 0.0f);
	Character->SetActorLocationAndRotation(
		State.ExitLocation,
		ExitRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Character->SetActorEnableCollision(true);

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->SetComponentTickEnabled(true);
		Movement->SetDefaultMovementMode();
	}

	Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	bSeatedApplied = false;

	if (APDPlayerController* PlayerController = GetLocalPlayerController())
	{
		PlayerController->EndVehicleView();
	}
}
