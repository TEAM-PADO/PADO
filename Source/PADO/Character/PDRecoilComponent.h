#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDRecoilComponent.generated.h"

class APDWorldItemActor;
class UPDItemRecoilTrait;

/**
 * 발사한 만큼 컨트롤 회전을 밀어 올리고 다시 복원한다.
 *
 * 로컬 조종 클라이언트에서만 동작한다. 컨트롤 회전은 이동 예측의 ServerMove에
 * 실려 서버로 가므로 별도 복제가 필요 없다. 그래서 반동은 새 복제 시스템이
 * 아니라 입력 조작이다.
 *
 * 무기별 수치는 `UPDItemRecoilTrait`에 있고, 여기 있는 값은 무기와 무관하게
 * 적용되는 게임 규칙이다.
 */
UCLASS(ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDRecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDRecoilComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 플레이어가 마우스로 넣은 회전량이다. 반동을 눌러 상쇄한 만큼 차감한다. */
	void NotifyLookInput(const FVector2D& LookInput);

	/** 아직 화면에 실리지 않았거나 복원 중인 반동이 있는지. 테스트용이다. */
	UFUNCTION(BlueprintPure, Category = "PD|Recoil")
	bool HasActiveRecoil() const;

protected:
	/** 반동이 화면에 실리는 속도다. 낮으면 물컹하고 높으면 탁 친다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.1", UIMin = "5.0", UIMax = "40.0"))
	float InterpSpeed = 20.0f;

	/** 자세별 반동 완화다. 무기 속성이 아니라 게임 규칙이라 여기 둔다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings|Stance",
		meta = (ClampMin = "0.0", UIMax = "2.0"))
	float IdleMultiplier = 1.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings|Stance",
		meta = (ClampMin = "0.0", UIMax = "2.0"))
	float ShoulderedMultiplier = 0.8f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings|Stance",
		meta = (ClampMin = "0.0", UIMax = "2.0"))
	float AimingMultiplier = 0.6f;

private:
	/**
	 * 한 발이 나갈 때마다 불린다. 발사 시점 판단은 전부 Held Item 계층이
	 * 하고, 여기서는 받은 만큼만 쌓는다. 탄약·재장전·쿨다운을 여기서 다시
	 * 보면 같은 게이트가 두 벌이 되어 반드시 어긋난다.
	 */
	void HandleLocalShotFired(APDWorldItemActor* Item);

	bool IsLocallyControlledOwner() const;

	/** 지금 들고 있는 무기의 반동 설정이다. 없으면 반동이 없는 아이템이다. */
	const UPDItemRecoilTrait* ResolveRecoilTrait() const;

	/** 소유자의 로컬 PlayerController다. 아니면 nullptr. */
	APlayerController* ResolveOwnerController() const;

	float ResolveStanceMultiplier() const;

	void ApplyPendingRecoil(float DeltaTime);
	void RecoverPendingRecoil(float DeltaTime);

	/**
	 * 복원에 쓰는 반동 설정이다. 발사를 멈춰도 복원은 이어져야 하는데 그때는
	 * 무기를 이미 놓았을 수 있으므로 따로 들고 있는다.
	 */
	TWeakObjectPtr<const UPDItemRecoilTrait> RecoveringTrait;

	/** 아직 컨트롤 회전에 실리지 않고 남아 있는 반동이다. X가 Yaw, Y가 Pitch. */
	FVector2D PendingRecoil = FVector2D::ZeroVector;

	/** 지금까지 컨트롤 회전에 실어 놓은 누적 반동이다. 복원 대상이며, 복원을 켠 무기에서만 쌓는다. */
	FVector2D AppliedRecoil = FVector2D::ZeroVector;

	/** 이번 연사에서 몇 발째인지. 스프레이 패턴 커브의 X축이다. */
	int32 ShotIndexInBurst = 0;

	/** 마지막 발사로부터 흐른 시간이다. 복원 지연 판정에 쓴다. */
	float TimeSinceLastShot = 0.0f;

	FDelegateHandle ShotFiredHandle;
};
