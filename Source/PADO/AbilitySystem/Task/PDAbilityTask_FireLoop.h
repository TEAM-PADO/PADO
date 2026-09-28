#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "PADO/AbilitySystem/Definition/PDFireActionDefinition.h"
#include "PDAbilityTask_FireLoop.generated.h"

class UPDGA_FireAction;

/**
 * 조종하는 머신에서 방아쇠 입력을 발사 모드대로 해석하고 발사 일정을 돌린다.
 *
 * 일정은 고정 시간축이다. 다음 발 시각에 발 간격을 더해 가고, 한 번에 둘 이상
 * 도래했으면 모두 쏜다. 그래서 연사력이 프레임레이트에 좌우되지 않는다. 쉬었다가
 * 다시 쏠 때는 밀린 발을 몰아 쏘지 않는다.
 *
 * 한 발의 게이트, 판정, 전송은 Fire Action이 한다. 여기서는 언제 쏠지만 정한다.
 * 쏠 발이 없을 때는 타이머도 걸지 않는다.
 */
UCLASS()
class PADO_API UPDAbilityTask_FireLoop : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UPDAbilityTask_FireLoop* Create(
		UPDGA_FireAction* OwningAbility,
		const UPDFireActionDefinition& Definition);

	void PressTrigger();
	void ReleaseTrigger();

	bool IsTriggerHeld() const { return bTriggerHeld; }

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	bool IsReady(double Now) const;

	/** 누르고 있는 자동 사격이나 진행 중인 점사처럼 다음 발이 예정돼 있는가. */
	bool IsRepeating() const;

	void FireShot(double ShotTime);
	void FireDueShots(double Now);
	void HandleScheduledShot();
	void ScheduleNextShot(double Now);

	/** 탄약 소진이나 재장전 요청으로 이번 방아쇠 입력이 끝났다. 다시 눌러야 쏜다. */
	void EndTriggerPull();

	void ClearScheduledShot();
	double GetWorldTime() const;

	TWeakObjectPtr<UPDGA_FireAction> FireAbility;

	/** Definition은 Grant 중에 바뀌지 않으므로 만들 때 한 번 읽어 둔다. */
	EPDFireMode FireMode = EPDFireMode::Automatic;
	double ShotInterval = 0.1;
	int32 BurstCount = 1;
	double BurstCooldown = 0.0;

	/** 다음 발을 쏠 수 있는 가장 이른 월드 시각이다. */
	double ReadyTime = TNumericLimits<double>::Lowest();

	bool bTriggerHeld = false;

	/** 이번 방아쇠 입력이 아직 유효한가. */
	bool bPullActive = false;

	int32 BurstShotsRemaining = 0;

	FTimerHandle ShotTimerHandle;
};
