#include "PADO/AbilitySystem/Task/PDAbilityTask_FireLoop.h"

#include "Engine/World.h"
#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"
#include "TimerManager.h"

namespace PDFireLoop
{
	/**
	 * 같은 시각끼리의 비교가 부동소수 오차로 막히지 않게 두는 여유다. 타이머와
	 * 월드 시간은 따로 누적되므로 예정 시각에 불려도 아주 조금 이를 수 있다.
	 */
	constexpr double ScheduleTimeTolerance = 1.0e-4;
}

UPDAbilityTask_FireLoop* UPDAbilityTask_FireLoop::Create(
	UPDGA_FireAction* OwningAbility,
	const UPDFireActionDefinition& Definition)
{
	UPDAbilityTask_FireLoop* Task =
		NewAbilityTask<UPDAbilityTask_FireLoop>(OwningAbility);
	Task->FireAbility = OwningAbility;
	Task->FireMode = Definition.FireMode;
	Task->ShotInterval = FMath::Max(
		Definition.ShotInterval,
		UPDFireActionDefinition::MinimumShotInterval);
	Task->BurstCount = FMath::Max(Definition.BurstCount, 1);
	Task->BurstCooldown = FMath::Max(Definition.BurstCooldown, 0.0f);
	return Task;
}

void UPDAbilityTask_FireLoop::PressTrigger()
{
	if (bTriggerHeld)
	{
		return;
	}

	bTriggerHeld = true;
	bPullActive = true;

	const double Now = GetWorldTime();
	const bool bBurstInProgress =
		FireMode == EPDFireMode::Burst && BurstShotsRemaining > 0;
	if (bBurstInProgress || !IsReady(Now))
	{
		// 자동은 준비되는 순간 쏜다. 반자동과 점사는 준비 전 입력을 버린다.
		// 모아 두면 누르지 않은 시점에 발이 나간다.
		ScheduleNextShot(Now);
		return;
	}

	if (FireMode == EPDFireMode::Burst)
	{
		BurstShotsRemaining = BurstCount;
	}

	// 첫 발은 일정을 기다리지 않고 누른 순간 쏘고 바로 보낸다.
	FireShot(Now);
	if (UPDGA_FireAction* OwningFireAction = FireAbility.Get())
	{
		OwningFireAction->FlushShotBatch();
	}
	ScheduleNextShot(Now);
}

void UPDAbilityTask_FireLoop::ReleaseTrigger()
{
	bTriggerHeld = false;

	// 점사는 떼도 끝까지 쏜다.
	if (!IsRepeating())
	{
		ClearScheduledShot();
	}
}

void UPDAbilityTask_FireLoop::OnDestroy(bool bInOwnerFinished)
{
	// 한 발의 결과로 Ability가 끝날 수 있다(무기를 떨어뜨리는 Fragment 등).
	// 그 발을 쏘던 반복이 이어서 쏘지 않게 상태도 함께 닫는다.
	bTriggerHeld = false;
	EndTriggerPull();
	Super::OnDestroy(bInOwnerFinished);
}

bool UPDAbilityTask_FireLoop::IsReady(double Now) const
{
	return ReadyTime <= Now + PDFireLoop::ScheduleTimeTolerance;
}

bool UPDAbilityTask_FireLoop::IsRepeating() const
{
	if (!bPullActive)
	{
		return false;
	}

	switch (FireMode)
	{
	case EPDFireMode::Automatic:
		return bTriggerHeld;

	case EPDFireMode::Burst:
		return BurstShotsRemaining > 0;

	case EPDFireMode::SemiAutomatic:
	default:
		return false;
	}
}

void UPDAbilityTask_FireLoop::FireShot(double ShotTime)
{
	UPDGA_FireAction* OwningFireAction = FireAbility.Get();
	if (!OwningFireAction)
	{
		EndTriggerPull();
		return;
	}

	switch (OwningFireAction->TryFireLocalShot())
	{
	case EPDFireShotOutcome::Fired:
		if (FireMode == EPDFireMode::Burst)
		{
			BurstShotsRemaining = FMath::Max(0, BurstShotsRemaining - 1);
			ReadyTime = ShotTime + (BurstShotsRemaining > 0
				? ShotInterval
				: FMath::Max(ShotInterval, BurstCooldown));
		}
		else
		{
			ReadyTime = ShotTime + ShotInterval;
		}
		break;

	case EPDFireShotOutcome::Blocked:
		// 막힌 자리는 건너뛰고 다음 자리에서 다시 본다. 방아쇠 상태는 그대로라서
		// 풀리면 이어서 쏜다. 반자동의 막힌 Press는 그냥 버린다.
		if (FireMode != EPDFireMode::SemiAutomatic)
		{
			ReadyTime = ShotTime + ShotInterval;
		}
		break;

	case EPDFireShotOutcome::Exhausted:
	default:
		EndTriggerPull();
		break;
	}
}

void UPDAbilityTask_FireLoop::FireDueShots(double Now)
{
	// 한 번에 둘 이상 도래했으면 모두 쏜다. 각 발은 예정된 시각을 쓴다.
	// 매 발이 다음 시각을 발 간격만큼 미루거나 방아쇠 입력을 끝내므로 반드시 멈춘다.
	while (IsRepeating() && IsReady(Now))
	{
		FireShot(FMath::Min(ReadyTime, Now));
	}
}

void UPDAbilityTask_FireLoop::HandleScheduledShot()
{
	ShotTimerHandle.Invalidate();

	const double Now = GetWorldTime();
	FireDueShots(Now);
	if (UPDGA_FireAction* OwningFireAction = FireAbility.Get())
	{
		OwningFireAction->FlushShotBatch();
	}
	ScheduleNextShot(Now);
}

void UPDAbilityTask_FireLoop::ScheduleNextShot(double Now)
{
	ClearScheduledShot();

	UWorld* World = GetWorld();
	if (!World || !IsRepeating())
	{
		return;
	}

	// 도래한 발은 이미 쐈다. 남은 것은 다음 자리까지의 대기다.
	const float Delay = FMath::Max(
		static_cast<float>(ReadyTime - Now),
		UE_KINDA_SMALL_NUMBER);
	World->GetTimerManager().SetTimer(
		ShotTimerHandle,
		this,
		&UPDAbilityTask_FireLoop::HandleScheduledShot,
		Delay,
		false);
}

void UPDAbilityTask_FireLoop::EndTriggerPull()
{
	bPullActive = false;
	BurstShotsRemaining = 0;
	ClearScheduledShot();
}

void UPDAbilityTask_FireLoop::ClearScheduledShot()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShotTimerHandle);
	}
	else
	{
		ShotTimerHandle.Invalidate();
	}
}

double UPDAbilityTask_FireLoop::GetWorldTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}
