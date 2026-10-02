#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PADO/AbilitySystem/Interface/PDAimStateProvider.h"
#include "PDCharacterMovementComponent.generated.h"

/**
 * 자세를 이동 예측에 실어 보내는 PADO 전용 무브먼트다.
 *
 * 최종 걷기 속도는 `어트리뷰트 MoveSpeed x 자세 배율`이다.
 * 어트리뷰트는 슬로우·헤이스트처럼 남이 걸어 주는 효과를 담고 서버가 확정한다.
 * 자세 배율은 본인 입력이 만드는 값이라 압축 플래그로 이동 데이터에 실어 보낸다.
 * 덕분에 소유 클라이언트가 즉시 반응하면서도 서버 재생 결과와 어긋나지 않는다.
 */
UCLASS()
class PADO_API UPDCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPDCharacterMovementComponent();

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** 로컬 입력이 설정한다. 압축 플래그로 서버에 전달된다. */
	void SetWantsToSprint(bool bNewWantsToSprint);
	bool WantsToSprint() const { return bWantsToSprint != 0; }

	void SetAimState(EPDAimState NewAimState);
	EPDAimState GetAimState() const { return AimState; }

	/** 조준 중이 아니고 걷고 있을 때만 달릴 수 있다. */
	UFUNCTION(BlueprintPure, Category = "PD|Movement")
	bool CanSprint() const;

	/** 지금 실제로 달리는 중인지 본다. 애니메이션이 이 값을 쓴다. */
	UFUNCTION(BlueprintPure, Category = "PD|Movement")
	bool IsSprinting() const;

	/** 자세 배율이다. 어트리뷰트 속도에 곱한다. */
	UFUNCTION(BlueprintPure, Category = "PD|Movement")
	float GetStanceSpeedMultiplier() const;

	/** 어트리뷰트에서 읽은 기본 속도를 캐릭터가 밀어 넣는다. */
	void SetAttributeMoveSpeed(float NewMoveSpeed);

	UFUNCTION(BlueprintPure, Category = "PD|Movement")
	float GetAttributeMoveSpeed() const { return AttributeMoveSpeed; }

	/**
	 * 걷다가 부딪힌 물리 대상을 몸으로 밀 수 있는지 본다. 탈것은 밀지 않는다.
	 * 엔진의 미는 힘은 질량에 비례하지 않아 무거운 차도 계속 밀면 굴러가고,
	 * 몸을 먼저 움직이는 클라이언트만 차를 밀어 서버와 어긋난다.
	 */
	static bool CanPushImpactedActor(const AActor* ImpactedActor);

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PD|Movement|Stance",
		meta = (ClampMin = "0.0"))
	float SprintSpeedMultiplier = 1.6f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PD|Movement|Stance",
		meta = (ClampMin = "0.0"))
	float ShoulderedSpeedMultiplier = 0.75f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PD|Movement|Stance",
		meta = (ClampMin = "0.0"))
	float AimingSpeedMultiplier = 0.5f;

protected:
	virtual void ApplyImpactPhysicsForces(
		const FHitResult& Impact,
		const FVector& ImpactAcceleration,
		const FVector& ImpactVelocity) override;

	/** 어트리뷰트를 아직 못 읽었을 때 쓰는 값이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "PD|Movement|Stance",
		meta = (ClampMin = "0.0"))
	float AttributeMoveSpeed = 500.0f;

	/** 로컬 입력 의도다. 압축 플래그로 전달돼 서버가 같은 값으로 재생한다. */
	uint8 bWantsToSprint : 1;

	/** 압축 플래그 2비트로 전달한다. */
	EPDAimState AimState = EPDAimState::Idle;

private:
	friend class FPDSavedMove;
};

/** 자세를 이동 기록에 함께 저장해 서버 재생과 로컬 재예측이 일치하게 한다. */
class PADO_API FPDSavedMove : public FSavedMove_Character
{
public:
	using Super = FSavedMove_Character;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(
		const FSavedMovePtr& NewMove,
		ACharacter* InCharacter,
		float MaxDelta) const override;
	virtual void SetMoveFor(
		ACharacter* C,
		float InDeltaTime,
		const FVector& NewAccel,
		FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* C) override;

	uint8 bSavedWantsToSprint : 1;
	EPDAimState SavedAimState = EPDAimState::Idle;
};

/** FPDSavedMove를 발급하기 위한 클라이언트 예측 데이터다. */
class PADO_API FPDNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
public:
	using Super = FNetworkPredictionData_Client_Character;

	explicit FPDNetworkPredictionData_Client(
		const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};
