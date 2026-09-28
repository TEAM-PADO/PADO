#pragma once

#include "CoreMinimal.h"
#include "PADO/Item/Trait/PDItemTrait.h"
#include "PDItemRecoilTrait.generated.h"

class UCurveFloat;

/**
 * 총기처럼 발사할 때마다 조준이 밀리는 아이템에 추가한다.
 *
 * 여기 있는 값은 전부 조준 반동, 즉 컨트롤 회전에 실리는 게임플레이 수치다.
 * 총기가 들썩이는 모션은 애니메이션이 표현하므로 여기에 적지 않는다.
 * 화면이 얼마나 부드럽게 따라가는지는 무기가 아니라 게임 규칙이므로
 * `UPDRecoilComponent`에 있다.
 */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Recoil"))
class PADO_API UPDItemRecoilTrait : public UPDItemTrait
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;

	/** 한 발당 조준점이 올라가는 각도다. 실제로 제일 많이 만지는 값이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.0", UIMax = "5.0", Units = "deg"))
	float PitchPerShot = 1.2f;

	/** 수직 반동에 더하는 무작위 편차 폭이다. 0이면 매 발 같은 값이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.0", UIMax = "2.0", Units = "deg"))
	float PitchVariance = 0.2f;

	/** 한 발당 좌우로 밀리는 각도 범위다. 최솟값이 최댓값보다 클 수 없다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (UIMin = "-2.0", UIMax = "0.0", Units = "deg"))
	float YawPerShotMin = -0.4f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (UIMin = "0.0", UIMax = "2.0", Units = "deg"))
	float YawPerShotMax = 0.4f;

	/**
	 * 발사를 멈춘 뒤 원래 조준점으로 돌아올지 정한다.
	 *
	 * 끄면 밀린 조준점이 그대로 새 조준점이 된다. 연사를 다 쏜 뒤 한참 늦게
	 * 조준이 내려오는 게 어색한 무기에서 끈다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	bool bEnableRecovery = true;

	/**
	 * 발사를 멈춘 뒤 복원이 시작되기까지 기다리는 시간이다.
	 * 연사가 끊겼다고 보는 기준이기도 해서, 복원을 꺼도 스프레이 패턴은
	 * 이 시간이 지나면 처음부터 다시 센다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ClampMin = "0.0", UIMax = "1.0", Units = "s"))
	float RecoveryDelay = 0.15f;

	/** 원래 조준점으로 돌아오는 속도다. 클수록 빨리 복원한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (EditCondition = "bEnableRecovery", ClampMin = "0.0", UIMax = "30.0"))
	float RecoverySpeed = 8.0f;

	/**
	 * 연사에서 n번째 탄의 반동 배율이다. X축이 탄 번호, Y축이 배율이다.
	 *
	 * 비워 두면 매 발 같은 수치에 무작위 편차만 적용한다. 외울 수 있는 반동
	 * 패턴이 필요해지면 그때 커브를 꽂는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UCurveFloat> SprayPattern;

	/** 연사 ShotIndex번째(0부터) 탄에 적용할 배율이다. 커브가 없으면 1이다. */
	float GetSprayMultiplier(int32 ShotIndex) const;
};
