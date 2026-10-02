#include "PADO/Vehicle/Component/PDVehicleOccupancyComponent.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Vehicle/Component/PDVehicleOccupantComponent.h"
#include "PADO/Vehicle/Component/PDVehicleSeatComponent.h"
#include "PADO/Vehicle/Interface/PDControllableVehicle.h"

namespace PDVehicleOccupancy
{
	/** 모든 하차 지점이 막혔을 때 탈것 위로 내릴 여유 높이다. */
	constexpr float AboveVehicleMargin = 20.0f;

	/**
	 * 이보다 느리면 멈춘 차로 보고 속도를 넘기지 않는다(cm/s). 서 있는 차체의
	 * 미세한 떨림이 내린 몸을 낙하 상태로 만들지 않게 한다.
	 */
	constexpr float MinInheritedExitSpeed = 10.0f;
}

UPDVehicleOccupancyComponent::UPDVehicleOccupancyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UPDVehicleOccupancyComponent::TryEnter(
	APDCharacterBase* Character,
	UPDVehicleSeatComponent* PreferredSeat)
{
	AActor* Vehicle = GetOwner();
	UPDVehicleOccupantComponent* Occupant =
		Character ? Character->GetVehicleOccupantComponent() : nullptr;
	if (!Vehicle || !Vehicle->HasAuthority() || !Occupant || Occupant->IsSeated())
	{
		return false;
	}

	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& Seats = GetSeats();
	EnsureOccupantSlots();
	if (Seats.IsEmpty())
	{
		return false;
	}

	int32 StartIndex = PreferredSeat ? Seats.IndexOfByKey(PreferredSeat) : INDEX_NONE;
	if (StartIndex == INDEX_NONE)
	{
		StartIndex = FMath::Max(
			0,
			Seats.IndexOfByKey(FindNearestSeat(Character->GetActorLocation())));
	}

	for (int32 Offset = 0; Offset < Seats.Num(); ++Offset)
	{
		const int32 SeatIndex = (StartIndex + Offset) % Seats.Num();
		if (IsValid(SeatOccupants[SeatIndex]))
		{
			continue;
		}

		OccupySeat(SeatIndex, *Character);
		return true;
	}

	return false;
}

bool UPDVehicleOccupancyComponent::TryExit(APDCharacterBase* Character)
{
	const AActor* Vehicle = GetOwner();
	const int32 SeatIndex = FindSeatIndex(Character);
	UPDVehicleOccupantComponent* Occupant =
		Character ? Character->GetVehicleOccupantComponent() : nullptr;
	if (!Vehicle || !Vehicle->HasAuthority() || SeatIndex == INDEX_NONE || !Occupant)
	{
		return false;
	}

	// 하차 지점은 좌석을 비우기 전에 고른다. 다른 탑승자의 하차 지점도 후보라서
	// 좌석 순서를 그대로 쓴다.
	const FVector ExitLocation = ResolveExitLocation(*Character, SeatIndex);
	const FVector ExitVelocity = ResolveExitVelocity(SeatIndex);
	ClearSeat(SeatIndex);
	Occupant->ExitSeat(ExitLocation, ExitVelocity);
	return true;
}

bool UPDVehicleOccupancyComponent::TrySwitchSeat(
	APDCharacterBase* Character,
	UPDVehicleSeatComponent* TargetSeat)
{
	const AActor* Vehicle = GetOwner();
	const int32 CurrentIndex = FindSeatIndex(Character);
	const int32 TargetIndex = TargetSeat ? GetSeats().IndexOfByKey(TargetSeat) : INDEX_NONE;
	if (!Vehicle || !Vehicle->HasAuthority() || CurrentIndex == INDEX_NONE ||
		!SeatOccupants.IsValidIndex(TargetIndex) || TargetIndex == CurrentIndex ||
		IsValid(SeatOccupants[TargetIndex]))
	{
		return false;
	}

	// 조종석을 비우는 쪽을 먼저 처리한다. 조종석으로 옮기면 그다음에 권한을 넘긴다.
	ClearSeat(CurrentIndex);
	OccupySeat(TargetIndex, *Character);
	return true;
}

UPDVehicleSeatComponent* UPDVehicleOccupancyComponent::FindSeatByNumber(
	int32 SeatNumber) const
{
	for (UPDVehicleSeatComponent* Seat : GetSeats())
	{
		if (Seat->GetSeatNumber() == SeatNumber)
		{
			return Seat;
		}
	}
	return nullptr;
}

UPDVehicleSeatComponent* UPDVehicleOccupancyComponent::FindNextFreeSeat(
	const APDCharacterBase* Character) const
{
	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& Seats = GetSeats();
	const int32 CurrentIndex = FindSeatIndex(Character);
	if (CurrentIndex == INDEX_NONE)
	{
		return nullptr;
	}

	for (int32 Offset = 1; Offset < Seats.Num(); ++Offset)
	{
		const int32 SeatIndex = (CurrentIndex + Offset) % Seats.Num();
		if (!SeatOccupants.IsValidIndex(SeatIndex) || !IsValid(SeatOccupants[SeatIndex]))
		{
			return Seats[SeatIndex];
		}
	}
	return nullptr;
}

void UPDVehicleOccupancyComponent::ReleaseOccupant(APDCharacterBase* Character)
{
	const AActor* Vehicle = GetOwner();
	const int32 SeatIndex = FindSeatIndex(Character);
	if (Vehicle && Vehicle->HasAuthority() && SeatIndex != INDEX_NONE)
	{
		ClearSeat(SeatIndex);
	}
}

bool UPDVehicleOccupancyComponent::HasFreeSeat() const
{
	const int32 SeatCount = GetSeats().Num();
	for (int32 SeatIndex = 0; SeatIndex < SeatCount; ++SeatIndex)
	{
		// 서버가 아직 슬롯을 만들지 않았거나 복제가 늦었으면 빈 좌석이다.
		if (!SeatOccupants.IsValidIndex(SeatIndex) || !IsValid(SeatOccupants[SeatIndex]))
		{
			return true;
		}
	}
	return false;
}

APDCharacterBase* UPDVehicleOccupancyComponent::GetSeatOccupant(
	const UPDVehicleSeatComponent* Seat) const
{
	const int32 SeatIndex = GetSeats().IndexOfByKey(Seat);
	return SeatOccupants.IsValidIndex(SeatIndex) ? SeatOccupants[SeatIndex].Get() : nullptr;
}

UPDVehicleSeatComponent* UPDVehicleOccupancyComponent::FindNearestSeat(
	const FVector& Location) const
{
	UPDVehicleSeatComponent* NearestSeat = nullptr;
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (UPDVehicleSeatComponent* Seat : GetSeats())
	{
		const double DistanceSquared =
			FVector::DistSquared(Location, Seat->GetComponentLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestSeat = Seat;
		}
	}
	return NearestSeat;
}

const TArray<TObjectPtr<UPDVehicleSeatComponent>>& UPDVehicleOccupancyComponent::GetSeats() const
{
	if (!bSeatsCached)
	{
		if (const AActor* Vehicle = GetOwner())
		{
			TArray<UPDVehicleSeatComponent*> Seats;
			Vehicle->GetComponents<UPDVehicleSeatComponent>(Seats);
			Seats.Sort([](const UPDVehicleSeatComponent& Left, const UPDVehicleSeatComponent& Right)
			{
				return Left.GetSeatNumber() != Right.GetSeatNumber()
					? Left.GetSeatNumber() < Right.GetSeatNumber()
					: Left.GetFName().LexicalLess(Right.GetFName());
			});
			CachedSeats.Reset(Seats.Num());
			CachedSeats.Append(Seats);
			bSeatsCached = true;
		}
	}
	return CachedSeats;
}

void UPDVehicleOccupancyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 탈것이 사라지면 탑승자를 모두 내린다. 몸이 함께 사라지는 중이면 좌석만 비운다.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (int32 SeatIndex = 0; SeatIndex < SeatOccupants.Num(); ++SeatIndex)
		{
			APDCharacterBase* Character = SeatOccupants[SeatIndex];
			if (IsValid(Character))
			{
				TryExit(Character);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UPDVehicleOccupancyComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDVehicleOccupancyComponent, SeatOccupants);
}

void UPDVehicleOccupancyComponent::EnsureOccupantSlots()
{
	const int32 SeatCount = GetSeats().Num();
	if (SeatOccupants.Num() != SeatCount)
	{
		SeatOccupants.SetNum(SeatCount);
	}
}

int32 UPDVehicleOccupancyComponent::FindSeatIndex(const APDCharacterBase* Character) const
{
	return Character ? SeatOccupants.IndexOfByKey(Character) : INDEX_NONE;
}

void UPDVehicleOccupancyComponent::OccupySeat(int32 SeatIndex, APDCharacterBase& Character)
{
	AActor* Vehicle = GetOwner();
	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& Seats = GetSeats();
	UPDVehicleOccupantComponent* Occupant = Character.GetVehicleOccupantComponent();
	if (!Vehicle || !Occupant || !SeatOccupants.IsValidIndex(SeatIndex) ||
		!Seats.IsValidIndex(SeatIndex))
	{
		return;
	}

	UPDVehicleSeatComponent* Seat = Seats[SeatIndex];
	SeatOccupants[SeatIndex] = &Character;
	Occupant->EnterSeat(Seat);

	if (Seat->IsDriverSeat())
	{
		if (IPDControllableVehicle* Controllable = Cast<IPDControllableVehicle>(Vehicle))
		{
			Controllable->SetVehicleController(Character.GetController());
		}
	}

	Vehicle->ForceNetUpdate();
}

void UPDVehicleOccupancyComponent::ClearSeat(int32 SeatIndex)
{
	AActor* Vehicle = GetOwner();
	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& Seats = GetSeats();
	if (!Vehicle || !SeatOccupants.IsValidIndex(SeatIndex) || !Seats.IsValidIndex(SeatIndex))
	{
		return;
	}

	SeatOccupants[SeatIndex] = nullptr;
	if (Seats[SeatIndex]->IsDriverSeat())
	{
		if (IPDControllableVehicle* Controllable = Cast<IPDControllableVehicle>(Vehicle))
		{
			Controllable->SetVehicleController(nullptr);
		}
	}

	Vehicle->ForceNetUpdate();
}

FVector UPDVehicleOccupancyComponent::ResolveExitLocation(
	const APDCharacterBase& Character,
	int32 SeatIndex) const
{
	// 앉았던 좌석의 하차 지점이 먼저고, 막혔으면 다른 좌석의 하차 지점을 순서대로 본다.
	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& Seats = GetSeats();
	for (int32 Offset = 0; Offset < Seats.Num(); ++Offset)
	{
		const FVector Candidate =
			Seats[(SeatIndex + Offset) % Seats.Num()]->GetExitLocation();
		if (!IsExitBlocked(Character, Candidate))
		{
			return Candidate;
		}
	}

	// 모두 막혔으면 탈것 위로 내린다. 몸이 탈것 안에 갇히는 것보다 낫다.
	const FBox VehicleBounds = GetOwner()->GetComponentsBoundingBox(true);
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.0f;
	const FVector Center = VehicleBounds.IsValid
		? VehicleBounds.GetCenter()
		: GetOwner()->GetActorLocation();
	const float Top = VehicleBounds.IsValid ? VehicleBounds.Max.Z : Center.Z;
	return FVector(
		Center.X,
		Center.Y,
		Top + HalfHeight + PDVehicleOccupancy::AboveVehicleMargin);
}

FVector UPDVehicleOccupancyComponent::ResolveExitVelocity(int32 SeatIndex) const
{
	const AActor* Vehicle = GetOwner();
	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& Seats = GetSeats();
	if (!Vehicle || !Seats.IsValidIndex(SeatIndex))
	{
		return FVector::ZeroVector;
	}

	// 차체가 돌고 있으면 좌석 자리의 속도는 차체 중심의 속도와 다르다.
	const UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent());
	const FVector Velocity = Body && Body->IsSimulatingPhysics()
		? Body->GetPhysicsLinearVelocityAtPoint(Seats[SeatIndex]->GetComponentLocation())
		: Vehicle->GetVelocity();
	return Velocity.SizeSquared() < FMath::Square(PDVehicleOccupancy::MinInheritedExitSpeed)
		? FVector::ZeroVector
		: Velocity;
}

bool UPDVehicleOccupancyComponent::IsExitBlocked(
	const APDCharacterBase& Character,
	const FVector& Location) const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}

	// 탑승 중에는 몸의 충돌이 꺼져 있으므로 캡슐 모양만 빌려 검사한다.
	FCollisionQueryParams QueryParams(TEXT("PDVehicleExit"), false, &Character);
	return World->OverlapBlockingTestByChannel(
		Location,
		FQuat::Identity,
		Capsule->GetCollisionObjectType(),
		FCollisionShape::MakeCapsule(
			Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleHalfHeight()),
		QueryParams,
		FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()));
}
