#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
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
		return ExitAuthority();
	}

	ServerRequestExit();
	return true;
}

void UPDVehicleOccupantComponent::ServerRequestExit_Implementation()
{
	ExitAuthority();
}

bool UPDVehicleOccupantComponent::RequestSwitchSeat(int32 SeatNumber)
{
	AActor* Owner = GetOwner();
	if (!Owner || !IsSeated() || State.Seat->GetSeatNumber() == SeatNumber)
	{
		return false;
	}

	if (Owner->HasAuthority())
	{
		return SwitchSeatAuthority(SeatNumber);
	}

	ServerRequestSwitchSeat(SeatNumber);
	return true;
}

void UPDVehicleOccupantComponent::ServerRequestSwitchSeat_Implementation(int32 SeatNumber)
{
	SwitchSeatAuthority(SeatNumber);
}

bool UPDVehicleOccupantComponent::RequestSwitchToNextSeat()
{
	AActor* Owner = GetOwner();
	if (!Owner || !IsSeated())
	{
		return false;
	}

	if (Owner->HasAuthority())
	{
		return SwitchToNextSeatAuthority();
	}

	ServerRequestSwitchToNextSeat();
	return true;
}

void UPDVehicleOccupantComponent::ServerRequestSwitchToNextSeat_Implementation()
{
	SwitchToNextSeatAuthority();
}

bool UPDVehicleOccupantComponent::ExitAuthority()
{
	APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	UPDVehicleOccupancyComponent* Occupancy = ResolveOccupancy();
	return Character && Character->HasAuthority() && Occupancy &&
		Occupancy->TryExit(Character);
}

bool UPDVehicleOccupantComponent::SwitchSeatAuthority(int32 SeatNumber)
{
	APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	UPDVehicleOccupancyComponent* Occupancy = ResolveOccupancy();
	return Character && Character->HasAuthority() && Occupancy &&
		Occupancy->TrySwitchSeat(Character, Occupancy->FindSeatByNumber(SeatNumber));
}

bool UPDVehicleOccupantComponent::SwitchToNextSeatAuthority()
{
	APDCharacterBase* Character = Cast<APDCharacterBase>(GetOwner());
	UPDVehicleOccupancyComponent* Occupancy = ResolveOccupancy();
	return Character && Character->HasAuthority() && Occupancy &&
		Occupancy->TrySwitchSeat(Character, Occupancy->FindNextFreeSeat(Character));
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

void UPDVehicleOccupantComponent::ExitSeat(
	const FVector& ExitLocation,
	const FVector& ExitVelocity)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	State.Seat = nullptr;
	State.ExitLocation = ExitLocation;
	State.ExitVelocity = ExitVelocity;
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

	// ASC는 몸보다 오래 살 수 있다. 붙여 둔 손 사용 불가 태그는 모든 머신에서 뗀다.
	UnblockHands();

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

		// 앉으면 손을 쓰지 않는다. 진행 중이던 행동은 서버와 조종하는 머신이 각자
		// 끊는다. 매핑을 바꾸는 순간 입력 해제가 온다는 보장이 없다.
		BlockHands();
		if (APDCharacterBase* CharacterBase = Cast<APDCharacterBase>(Character);
			CharacterBase && (CharacterBase->HasAuthority() || CharacterBase->IsLocallyControlled()))
		{
			CharacterBase->InterruptHandActions();
		}
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
	// 같은 탈것 안에서 좌석을 옮겼으면 좌석 역할에 맞는 입력으로만 바뀐다.
	if (APDPlayerController* PlayerController = GetLocalPlayerController())
	{
		PlayerController->BeginVehicleView(Seat);
	}

	OnSeatChanged.Broadcast();
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

	// 죽은 몸은 다시 걷지 않는다. 래그돌이 이어받도록 차의 속도만 남긴다.
	const APDCharacterBase* CharacterBase = Cast<APDCharacterBase>(Character);
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (Movement && CharacterBase && CharacterBase->IsDead())
	{
		Movement->Velocity = State.ExitVelocity;
	}
	else if (Movement)
	{
		// 탑승 중에는 소유 클라이언트가 이동을 보내지 않는다. 그대로 두면 서버는 내리는
		// 순간 앉아 있던 시간 전체를 늦은 클라이언트의 공백으로 보고 한꺼번에
		// 시뮬레이션하며, 그만큼 이후 클라이언트 이동을 오래된 것으로 거부한다
		// (APlayerController::TickActor의 ForcePositionUpdate). 서버가 마지막으로 이동을
		// 받은 시각을 내린 순간으로 옮겨 그 공백을 없앤다. 클라이언트의 이동 시계는
		// 탑승 중 멈춰 있어 이어지므로 시계 기준은 건드리지 않는다.
		if (Character->HasAuthority() && !Character->IsLocallyControlled())
		{
			if (FNetworkPredictionData_Server_Character* ServerData =
				Movement->HasPredictionData_Server()
					? Movement->GetPredictionData_Server_Character()
					: nullptr)
			{
				ServerData->ServerTimeStamp =
					static_cast<float>(Character->GetWorld()->GetTimeSeconds());
				ServerData->ResetForcedUpdateState();
			}
		}

		Movement->SetComponentTickEnabled(true);
		Movement->SetDefaultMovementMode();

		// 달리던 차의 속도를 이어받는다. 공중에서 시작하고 착지와 제동은 무브먼트가
		// 정한다. 소유 클라이언트도 서버와 같은 위치·속도에서 시작한다.
		if (!State.ExitVelocity.IsZero())
		{
			Movement->Velocity = State.ExitVelocity;
			Movement->SetMovementMode(MOVE_Falling);
		}
	}

	Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	bSeatedApplied = false;
	UnblockHands();

	if (APDPlayerController* PlayerController = GetLocalPlayerController())
	{
		PlayerController->EndVehicleView();
	}

	OnSeatChanged.Broadcast();
}

void UPDVehicleOccupantComponent::BlockHands()
{
	if (HandsBlockedAbilitySystem.IsValid())
	{
		return;
	}

	// 클라이언트에서는 ASC가 몸보다 늦게 복제될 수 있다. 그때 이 머신은 예측만
	// 막지 못하고, 거부는 서버가 한다.
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!AbilitySystem)
	{
		return;
	}

	AbilitySystem->AddLooseGameplayTag(TAG_PD_State_HandsBlocked);
	HandsBlockedAbilitySystem = AbilitySystem;
}

void UPDVehicleOccupantComponent::UnblockHands()
{
	// 같은 태그를 다른 상태가 붙였을 수 있으므로 개수를 0으로 만들지 않고 붙인 만큼만 뗀다.
	if (UAbilitySystemComponent* AbilitySystem = HandsBlockedAbilitySystem.Get())
	{
		AbilitySystem->RemoveLooseGameplayTag(TAG_PD_State_HandsBlocked);
	}
	HandsBlockedAbilitySystem.Reset();
}
