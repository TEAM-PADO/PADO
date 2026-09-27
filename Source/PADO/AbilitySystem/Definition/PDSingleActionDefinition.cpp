#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"

#include "PADO/AbilitySystem/Ability/PDGA_Action.h"
#include "PADO/AbilitySystem/Fragment/PDThrowProjectileFragment.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"

TSubclassOf<UPDGA_Base> UPDSingleActionDefinition::GetAbilityClass() const
{
	return UPDGA_Action::StaticClass();
}

bool UPDSingleActionDefinition::ExecutesOnInputRelease() const
{
	return FindChargedThrowProjectileFragment() != nullptr;
}

const UPDThrowProjectileFragment*
UPDSingleActionDefinition::FindChargedThrowProjectileFragment() const
{
	for (const FPDActionHookStruct& Hook : ActionHooks)
	{
		for (const UPDActionFragment* Fragment : Hook.Fragments)
		{
			const UPDThrowProjectileFragment* ThrowFragment =
				Cast<UPDThrowProjectileFragment>(Fragment);
			if (ThrowFragment && ThrowFragment->LaunchConfig.bEnableCharge)
			{
				return ThrowFragment;
			}
		}
	}

	return nullptr;
}

float UPDSingleActionDefinition::GetAutomaticFireInterval() const
{
	return bAutomatic && ActionCooldown.IsEnabled()
		? ActionCooldown.Duration
		: 0.0f;
}

float UPDSingleActionDefinition::GetEnforcedCooldownDuration() const
{
	const float Interval = GetAutomaticFireInterval();
	return Interval > 0.0f
		? Interval * (1.0f - AutomaticFireCooldownTolerance)
		: ActionCooldown.Duration;
}

bool UPDSingleActionDefinition::ValidateLifecycle(FString& OutError) const
{
	// 쿨다운이 없으면 서버가 발사 간격을 강제하지 못한다. 클라이언트가 보내는
	// 만큼 나가므로 조작에 열린다.
	if (bAutomatic && !ActionCooldown.IsEnabled())
	{
		OutError = TEXT("자동 발사 Single Action에는 발사 간격을 정할 ActionCooldown이 필요합니다.");
		return false;
	}

	// 충전은 Release에서 실행하므로 누르는 동안 반복한다는 개념과 맞지 않는다.
	if (bAutomatic && ExecutesOnInputRelease())
	{
		OutError = TEXT("충전 Action은 자동 발사로 설정할 수 없습니다.");
		return false;
	}

	if (ActionTargeting &&
		ActionTargeting->IsA<UPDTraceWindowTargeting>() &&
		!IsValid(ActionMontage.Montage))
	{
		OutError = TEXT("TraceWindow Targeting을 쓰는 Single Action에는 NotifyState를 담을 Montage가 필요합니다.");
		return false;
	}

	int32 ChargedThrowCount = 0;
	for (const FPDActionHookStruct& Hook : ActionHooks)
	{
		for (const UPDActionFragment* Fragment : Hook.Fragments)
		{
			const UPDThrowProjectileFragment* ThrowFragment =
				Cast<UPDThrowProjectileFragment>(Fragment);
			if (ThrowFragment && ThrowFragment->LaunchConfig.bEnableCharge)
			{
				++ChargedThrowCount;
			}
		}
	}

	if (ChargedThrowCount > 1)
	{
		OutError = TEXT("Single Action 하나에는 충전 Throw Projectile Fragment를 하나만 둘 수 있습니다.");
		return false;
	}

	return true;
}
