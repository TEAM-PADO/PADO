#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "PADO/Interaction/Interface/PDInteractable.h"
#include "PADO/Vehicle/Interface/PDControllableVehicle.h"
#include "PDWheeledVehicle.generated.h"

class AController;
class APDPlayerController;
class UCameraComponent;
class UPrimitiveComponent;
class USpringArmComponent;
class UPDVehicleOccupancyComponent;
class UPDVehicleSeatComponent;

/**
 * 바퀴형 탈것이다. 주행 물리와 예측은 엔진의 Chaos 차량이 맡는다.
 *
 * 서버가 물리 권한을 갖고, 운전자 머신은 자기 입력을 즉시 예측한다. 어긋나면
 * 과거 물리 스텝부터 다시 계산한다(Physics Prediction, Resimulation).
 *
 * 운전자도 차량에 빙의하지 않는다. 조종석이 채워지면 서버가 Owner와
 * OverrideController를 운전자의 컨트롤러로 바꾼다. 캐릭터 빙의가 유지되므로
 * 캐릭터의 Ability System과 RPC 연결이 그대로 남는다.
 *
 * 탑승자는 좌석과 상관없이 모두 차량 카메라를 본다. 회전은 머신마다 그
 * 머신의 탑승자 시점을 따른다.
 */
UCLASS(Blueprintable)
class PADO_API APDWheeledVehicle
	: public AWheeledVehiclePawn
	, public IPDInteractable
	, public IPDControllableVehicle
{
	GENERATED_BODY()

public:
	explicit APDWheeledVehicle(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	/** 주체가 타고 있지 않고 빈 좌석이 있으면 탈 수 있다. */
	virtual bool CanInteract_Implementation(
		const FPDInteractionContextStruct& Context) const override;

	/** 조준한 좌석이 있으면 그 좌석을, 없으면 가장 가까운 좌석을 희망 좌석으로 탄다. */
	virtual bool Interact_Implementation(
		const FPDInteractionContextStruct& Context) override;

	virtual void SetVehicleController(AController* NewController) override;
	virtual AController* GetVehicleController() const override { return VehicleController; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle")
	UPDVehicleOccupancyComponent* GetOccupancyComponent() const { return OccupancyComponent; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Camera")
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Camera")
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_VehicleController(AController* OldController);

private:
	/**
	 * 입력을 누가 만드는지 정한다. 운전자가 없으면 서버가 중립 입력을 만들어 차가
	 * 관성으로 굴러가다 서고, 운전자가 있으면 그 머신이 만든다.
	 */
	void ApplyServerInputOwnership(bool bServerProducesInput);

	/** 이 머신의 플레이어가 조종을 얻거나 잃었으면 입력을 바꾸게 알린다. */
	void NotifyLocalControlChanged(
		AController* OldController,
		AController* NewController);

	/** 이 머신에서 이 차에 앉아 차량 카메라를 보고 있는 플레이어다. */
	APDPlayerController* FindLocalViewer() const;

	UPDVehicleSeatComponent* ResolvePreferredSeat(
		UPrimitiveComponent* AimedComponent) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Vehicle|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Vehicle|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Vehicle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDVehicleOccupancyComponent> OccupancyComponent;

	/** 조종석에 앉은 캐릭터의 컨트롤러다. 운전자 머신은 이 값으로 조종 시작을 안다. */
	UPROPERTY(ReplicatedUsing = OnRep_VehicleController)
	TObjectPtr<AController> VehicleController;
};
