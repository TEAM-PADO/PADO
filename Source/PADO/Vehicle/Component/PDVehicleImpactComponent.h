#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDVehicleImpactComponent.generated.h"

class APDCharacterBase;
class UAbilitySystemComponent;
class UPrimitiveComponent;
struct FHitResult;

/**
 * 탈것이 사람을 치는 것을 서버에서 판정한다. 차체가 캐릭터에 닿으면 차가 그 사람 쪽으로
 * 다가가던 속도로 피해를 정하고 밀어낸다. 아군도 같은 피해다.
 *
 * - 캐릭터 캡슐은 Probe 충돌이라 차를 막지 않고 접촉만 알린다(APDCharacterBase). 그 접촉이
 *   차체의 피격 알림으로 온다. 서버에서 차체의 모든 바디에 피격 알림을 켠다. 클라이언트
 *   물리는 되감기로 다시 계산되므로 서버 접촉만 쓴다.
 * - 탄 사람은 충돌이 꺼져 있어 닿지 않는다.
 * - 운전자가 있으면 운전자가 피해의 주체(Instigator)이고, 없으면 탈것 자신이다. 원인
 *   (EffectCauser)은 탈것이다.
 * - 한 번 부딪힐 때 접촉 알림이 여러 번 오므로 같은 사람은 RehitInterval 동안 다시 치지 않는다.
 * - 칠 때마다 서버에서 차를 감속시켜 무리를 밀고 나갈 때 버겁게 나아가게 한다. 운전자 화면은
 *   예측 보정으로 지연만큼 늦게 속도가 줄어든다(위치가 튀지 않는 것을 2026-10-02 MCP로 확인).
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDVehicleImpactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDVehicleImpactComponent();

	/**
	 * 서버에서 차체가 ContactPoint에서 캐릭터에 닿았을 때 부른다. 피해를 주거나 밀어냈으면
	 * true다. 차체 피격 알림이 부른다.
	 */
	bool HandleCharacterContact(APDCharacterBase& Character, const FVector& ContactPoint);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBodyHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	/**
	 * 차가 사람 쪽으로 다가가는 속도가 이보다 느리면 무시한다. 사람이 멈춘 차로 걸어
	 * 들어가는 것은 0이다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float MinImpactSpeed = 100.0f;

	/** 이 속도부터 피해를 준다. 이보다 느리면 밀어내기만 한다. 기본 15km/h다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float MinDamageSpeed = 417.0f;

	/** 이 속도에서 피해가 MaxDamage에 이른다. 그 사이는 속도에 비례한다. 기본 60km/h다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float MaxDamageSpeed = 1667.0f;

	/** 피해 상한이다. 기본은 플레이어 최대 체력이라 MaxDamageSpeed로 치면 즉사한다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0"))
	float MaxDamage = 100.0f;

	/** 밀어내는 수평 속도는 다가오던 속도에 이 값을 곱한다. 차보다 빨라야 다시 깔리지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0"))
	float KnockbackSpeedScale = 1.2f;

	/** 밀어낼 때 위로 띄우는 속도다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0", Units = "CentimetersPerSecond"))
	float KnockbackUpSpeed = 250.0f;

	/** 같은 사람을 다시 칠 수 있기까지의 시간이다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0", Units = "s"))
	float RehitInterval = 1.0f;

	/**
	 * 칠 때 차가 잃는 속도의 배율이다. 감속량은 다가오던 속도 × (상대 무게 ÷ 차 무게) × 이 값이고,
	 * 무게는 상대 캐릭터 무브먼트의 Mass와 차체의 물리 질량(모든 바디 합)이다. 픽업(약 2050kg)에
	 * 기본 2면 100kg짜리를 40km/h로 칠 때 약 4km/h 줄어든다. 0이면 감속하지 않는다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Impact", meta = (ClampMin = "0.0"))
	float ImpactSpeedLossScale = 2.0f;

private:
	/** 차체가 ContactPoint에서 Character 쪽으로 다가가는 속도다. 방향은 수평이다. */
	float ResolveImpactSpeed(
		const APDCharacterBase& Character,
		const FVector& ContactPoint,
		FVector& OutDirection) const;

	/** 운전자의 ASC다. 운전자가 없으면 탈것의 ASC다. */
	UAbilitySystemComponent* ResolveSourceAbilitySystem() const;

	/** 서버에서 사람을 밀어낸 반작용으로 차를 감속시킨다. Direction은 차에서 사람 쪽이다. */
	void ApplyImpactSpeedLoss(
		const APDCharacterBase& Character,
		const FVector& Direction,
		float ImpactSpeed) const;

	UPrimitiveComponent* GetBody() const;

	/** 사람마다 마지막으로 친 시각이다. 접촉이 올 때 오래된 기록을 지운다. */
	TMap<TWeakObjectPtr<const AActor>, double> LastImpactTimes;

	TWeakObjectPtr<UPrimitiveComponent> BoundBody;
};
