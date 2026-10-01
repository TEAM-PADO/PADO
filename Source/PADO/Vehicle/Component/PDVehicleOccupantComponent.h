#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "PDVehicleOccupantComponent.generated.h"

class ACharacter;
class APDPlayerController;
class UAbilitySystemComponent;
class UPDVehicleOccupancyComponent;
class UPDVehicleSeatComponent;

/** 탑승 상태다. 좌석과 마지막 하차 위치·속도를 한 구조체로 복제해 도착 순서가 어긋나지 않게 한다. */
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

	/**
	 * 내릴 때 이어받는 속도다. 앉았던 좌석 자리의 차체 속도이고, 멈춘 차면 0이다.
	 * 하차 위치와 함께 모든 머신이 같은 값으로 시작한다.
	 */
	UPROPERTY()
	FVector_NetQuantize10 ExitVelocity = FVector::ZeroVector;
};

/**
 * 탈것에 타는 쪽의 컴포넌트다. 탑승 상태를 복제하고, 앉으면 몸을 좌석에 붙이고
 * 이동·충돌·컨트롤러 회전 추종을 끈다. 앉아 있는 동안 손을 쓸 수 없다
 * (State.HandsBlocked). 이 머신의 플레이어면 시점도 탈것 카메라로 넘긴다.
 * 내리면 되돌리고 탈것의 속도를 이어받는다.
 *
 * 탑승, 좌석 이동, 하차의 확정은 탈것의 UPDVehicleOccupancyComponent가 서버에서
 * 한다. 요청 RPC는 이 컴포넌트가 보낸다. 탈것은 클라이언트가 소유하지 않을 수 있다.
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

	/**
	 * 입력 계층에서 부른다. 앉아 있으면 그 좌석 번호의 좌석으로 옮겨 달라고
	 * 서버에 요청한다. 좌석이 차 있으면 서버가 거부한다. 반환값은 요청했는지다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Vehicle")
	bool RequestSwitchSeat(int32 SeatNumber);

	/** 입력 계층에서 부른다. 앉아 있으면 다음 빈 좌석으로 옮겨 달라고 서버에 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "PD|Vehicle")
	bool RequestSwitchToNextSeat();

	/** 서버에서 좌석 구성 컴포넌트가 부른다. 앉히거나 다른 좌석으로 옮긴다. */
	void EnterSeat(UPDVehicleSeatComponent* Seat);

	/** 서버에서 좌석 구성 컴포넌트가 부른다. */
	void ExitSeat(const FVector& ExitLocation, const FVector& ExitVelocity);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void ServerRequestExit();

	UFUNCTION(Server, Reliable)
	void ServerRequestSwitchSeat(int32 SeatNumber);

	UFUNCTION(Server, Reliable)
	void ServerRequestSwitchToNextSeat();

	UFUNCTION()
	void OnRep_State();

private:
	/**
	 * 서버에서 요청을 확정한다. 권한이 없으면 아무것도 하지 않는다. RPC가 이
	 * 머신에서 실행되는 경우(에디터 스크립트 실행 등)에도 요청 함수로 되돌아가
	 * 다시 RPC를 보내지 않도록 RPC 구현은 이 함수들만 부른다.
	 */
	bool ExitAuthority();
	bool SwitchSeatAuthority(int32 SeatNumber);
	bool SwitchToNextSeatAuthority();

	UPDVehicleOccupancyComponent* ResolveOccupancy() const;
	ACharacter* GetOwnerCharacter() const;

	/** 이 머신이 몸을 조종하고 있으면 그 컨트롤러다. 탑승 시점은 이 컨트롤러만 바꾼다. */
	APDPlayerController* GetLocalPlayerController() const;
	void ApplyState();
	void ApplySeated(UPDVehicleSeatComponent& Seat);
	void ApplyUnseated();

	/** 이 머신의 ASC에 손 사용 불가 태그를 붙인다. 붙인 ASC를 기억해 두고 거기서만 뗀다. */
	void BlockHands();
	void UnblockHands();

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FPDVehicleOccupantStateStruct State;

	/** 앉기 전의 컨트롤러 회전 추종 설정이다. 내릴 때 되돌린다. */
	bool bSavedUseControllerRotationYaw = true;

	/** 이 머신에 앉은 상태를 적용했는지다. 복제가 같은 상태로 다시 와도 한 번만 적용한다. */
	bool bSeatedApplied = false;

	/**
	 * 손 사용 불가 태그를 붙인 ASC다. 플레이어의 ASC는 PlayerState에 있어 몸보다
	 * 오래 산다. 몸이 탄 채로 사라져도 여기서 떼야 다음 몸이 손을 쓸 수 있다.
	 */
	TWeakObjectPtr<UAbilitySystemComponent> HandsBlockedAbilitySystem;
};
