#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "PDVehicleOccupantComponent.generated.h"

class ACharacter;
class APDPlayerController;
class UPDVehicleOccupancyComponent;
class UPDVehicleSeatComponent;

/** 탑승 상태다. 좌석과 마지막 하차 위치를 한 구조체로 복제해 도착 순서가 어긋나지 않게 한다. */
USTRUCT()
struct FPDVehicleOccupantStateStruct
{
	GENERATED_BODY()

	/** 앉아 있는 좌석이다. null이면 타고 있지 않다. */
	UPROPERTY()
	TObjectPtr<UPDVehicleSeatComponent> Seat = nullptr;

	/**
	 * 마지막으로 내린 위치다. 좌석이 비면 모든 머신이 여기로 옮긴다. 소유
	 * 클라이언트도 서버와 같은 자리에서 다시 걷기 시작해 이동 보정이 생기지 않는다.
	 */
	UPROPERTY()
	FVector_NetQuantize ExitLocation = FVector::ZeroVector;
};

/**
 * 탈것에 타는 쪽의 컴포넌트다. 탑승 상태를 복제하고, 앉으면 몸을 좌석에 붙이고
 * 이동·충돌·컨트롤러 회전 추종을 끈다. 이 머신의 플레이어면 시점도 탈것
 * 카메라로 넘긴다. 내리면 되돌린다.
 *
 * 탑승과 하차의 확정은 탈것의 UPDVehicleOccupancyComponent가 서버에서 한다.
 * 하차 요청 RPC는 이 컴포넌트가 보낸다. 탈것은 클라이언트가 소유하지 않을 수 있다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDVehicleOccupantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDVehicleOccupantComponent();

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	bool IsSeated() const { return State.Seat != nullptr; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	UPDVehicleSeatComponent* GetCurrentSeat() const { return State.Seat; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	AActor* GetCurrentVehicle() const;

	/** 입력 계층에서 부른다. 앉아 있으면 서버에 하차를 요청한다. 반환값은 요청했는지다. */
	UFUNCTION(BlueprintCallable, Category = "PD|Vehicle")
	bool RequestExit();

	/** 서버에서 좌석 구성 컴포넌트가 부른다. */
	void EnterSeat(UPDVehicleSeatComponent* Seat);

	/** 서버에서 좌석 구성 컴포넌트가 부른다. */
	void ExitSeat(const FVector& ExitLocation);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void ServerRequestExit();

	UFUNCTION()
	void OnRep_State();

private:
	UPDVehicleOccupancyComponent* ResolveOccupancy() const;
	ACharacter* GetOwnerCharacter() const;

	/** 이 머신이 몸을 조종하고 있으면 그 컨트롤러다. 탑승 시점은 이 컨트롤러만 바꾼다. */
	APDPlayerController* GetLocalPlayerController() const;
	void ApplyState();
	void ApplySeated(UPDVehicleSeatComponent& Seat);
	void ApplyUnseated();

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FPDVehicleOccupantStateStruct State;

	/** 앉기 전의 컨트롤러 회전 추종 설정이다. 내릴 때 되돌린다. */
	bool bSavedUseControllerRotationYaw = true;

	/** 이 머신에 앉은 상태를 적용했는지다. 복제가 같은 상태로 다시 와도 한 번만 적용한다. */
	bool bSeatedApplied = false;
};
