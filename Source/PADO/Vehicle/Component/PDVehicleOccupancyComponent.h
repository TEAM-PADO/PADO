#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDVehicleOccupancyComponent.generated.h"

class APDCharacterBase;
class UPDVehicleSeatComponent;

/**
 * 탈것의 좌석 점유 상태다. 탑승·하차를 서버에서 확정하고 점유 상태를 복제한다.
 *
 * 좌석은 탈것에 붙은 UPDVehicleSeatComponent이고 좌석 번호 순서로 다룬다.
 * 조종석이 채워지거나 비면 탈것(IPDControllableVehicle)에 조종 권한을 넘기거나
 * 거둔다. 이동 방식은 모른다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDVehicleOccupancyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDVehicleOccupancyComponent();

	/**
	 * 서버에서 탑승을 확정한다. 희망 좌석이 비어 있으면 그 좌석에, 차 있으면 좌석
	 * 번호 순서대로 다음 빈 좌석에 앉힌다. 끝까지 가면 처음 좌석부터 본다.
	 * 희망 좌석이 없으면 주체와 가장 가까운 좌석부터 본다.
	 */
	bool TryEnter(APDCharacterBase* Character, UPDVehicleSeatComponent* PreferredSeat);

	/** 서버에서 하차를 확정한다. 캡슐이 들어가는 하차 지점을 고른다. */
	bool TryExit(APDCharacterBase* Character);

	/** 서버에서 몸이 사라질 때 좌석만 비운다. 하차 지점은 고르지 않는다. */
	void ReleaseOccupant(APDCharacterBase* Character);

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	bool HasFreeSeat() const;

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	APDCharacterBase* GetSeatOccupant(const UPDVehicleSeatComponent* Seat) const;

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	UPDVehicleSeatComponent* FindNearestSeat(const FVector& Location) const;

	/** 좌석 번호 순서로 정렬된 좌석이다. */
	const TArray<TObjectPtr<UPDVehicleSeatComponent>>& GetSeats() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void EnsureOccupantSlots();
	int32 FindSeatIndex(const APDCharacterBase* Character) const;
	void ClearSeat(int32 SeatIndex);
	FVector ResolveExitLocation(const APDCharacterBase& Character, int32 SeatIndex) const;
	bool IsExitBlocked(const APDCharacterBase& Character, const FVector& Location) const;

	/** 좌석 순서와 같은 순서의 탑승자다. 빈 좌석은 null이다. */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<APDCharacterBase>> SeatOccupants;

	/** 좌석은 실행 중에 바뀌지 않으므로 처음 조회할 때 모아 둔다. */
	UPROPERTY(Transient)
	mutable TArray<TObjectPtr<UPDVehicleSeatComponent>> CachedSeats;

	mutable bool bSeatsCached = false;
};
