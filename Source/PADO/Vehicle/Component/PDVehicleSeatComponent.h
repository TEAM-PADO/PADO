#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PDVehicleSeatComponent.generated.h"

UENUM(BlueprintType)
enum class EPDVehicleSeatRole : uint8
{
	/** 이 좌석에 앉으면 탈것의 조종 권한을 받는다. */
	Driver,

	/** 조종하지 않는다. 손에 든 것은 그대로 쓴다. */
	Passenger
};

/**
 * 탈것의 좌석 하나다. 탈것 Blueprint 뷰포트에서 앉은 캐릭터의 캡슐 중심에
 * 배치한다. 탑승한 캐릭터는 이 컴포넌트에 붙는다.
 *
 * 문 충돌체처럼 조준할 수 있는 컴포넌트를 이 좌석 아래에 붙이면, 그 컴포넌트를
 * 조준해 상호작용했을 때 이 좌석이 희망 좌석이 된다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDVehicleSeatComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPDVehicleSeatComponent();

	/** 런타임에 좌석 설정을 덮어써야 할 때 쓴다. 탈것이 좌석을 조회하기 전에 부른다. */
	void ConfigureSeat(int32 NewSeatNumber, EPDVehicleSeatRole NewSeatRole);

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Seat")
	int32 GetSeatNumber() const { return SeatNumber; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Seat")
	EPDVehicleSeatRole GetSeatRole() const { return SeatRole; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Seat")
	bool IsDriverSeat() const { return SeatRole == EPDVehicleSeatRole::Driver; }

	/** 이 좌석에서 내린 캐릭터의 캡슐 중심 위치다. */
	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Seat")
	FVector GetExitLocation() const;

private:
	/**
	 * 좌석 순서다. 희망 좌석이 차 있으면 이 번호 순서대로 다음 빈 좌석에 앉힌다.
	 * 컴포넌트 순서는 머신마다 같다고 보장되지 않으므로 번호로 정한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Vehicle|Seat", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 SeatNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Vehicle|Seat", meta = (AllowPrivateAccess = "true"))
	EPDVehicleSeatRole SeatRole = EPDVehicleSeatRole::Passenger;

	/**
	 * 좌석 기준 하차 지점(내린 캐릭터의 캡슐 중심)이다. 좌석 기준 -Y가 왼쪽이다.
	 * 막혀 있으면 다른 좌석의 하차 지점을 쓴다.
	 *
	 * 편집 위젯은 레벨 에디터에서 배치한 인스턴스의 좌석을 골랐을 때만 그려지고
	 * 차량 BP 에디터 뷰포트에는 나오지 않는다. BP에서는 디테일 패널에 숫자로 넣는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Vehicle|Seat", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector ExitOffset = FVector(0.0f, -150.0f, 0.0f);
};
