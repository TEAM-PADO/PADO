#include "PADO/AbilitySystem/Definition/PDFireActionDefinition.h"

#include "PADO/AbilitySystem/Ability/PDGA_FireAction.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"

TSubclassOf<UPDGA_Base> UPDFireActionDefinition::GetAbilityClass() const
{
	return UPDGA_FireAction::StaticClass();
}

bool UPDFireActionDefinition::ValidateLifecycle(FString& OutError) const
{
	// 발마다 그 자리에서 대상을 모은다. 몽타주 구간이나 활성화 이벤트가 정하는
	// 대상은 Fire Action에 없다.
	if (!ActionTargeting ||
		!ActionTargeting->IsA<UPDInstantActionTargeting>())
	{
		OutError = TEXT("Fire Action에는 Instant Targeting이 필요합니다.");
		return false;
	}

	if (ActionTargeting->RequiresActivationTarget())
	{
		OutError = TEXT("Fire Action은 활성화 대상이 필요한 Targeting을 쓸 수 없습니다.");
		return false;
	}

	if (!FMath::IsFinite(ShotInterval) || ShotInterval < MinimumShotInterval)
	{
		OutError = FString::Printf(
			TEXT("ShotInterval은 %.3f초 이상의 유한한 값이어야 합니다."),
			MinimumShotInterval);
		return false;
	}

	if (FireMode == EPDFireMode::Burst)
	{
		if (BurstCount < 1)
		{
			OutError = TEXT("점사 발 수는 1 이상이어야 합니다.");
			return false;
		}

		if (!FMath::IsFinite(BurstCooldown) || BurstCooldown < 0.0f)
		{
			OutError = TEXT("BurstCooldown은 0 이상의 유한한 값이어야 합니다.");
			return false;
		}
	}

	return true;
}
