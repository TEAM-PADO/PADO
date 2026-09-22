#include "PADO/AbilitySystem/Targeting/PDEventTargeting.h"

#include "GameFramework/Actor.h"

void UPDEventTargeting::GatherTargets(
	const FPDActionTargetingContext& Context,
	TArray<FPDActionTarget>& OutTargets) const
{
	// 타이밍 대기 중 대상이 파괴됐을 수 있다.
	if (!IsValid(Context.ActivationTarget))
	{
		return;
	}

	FPDActionTarget& Target = OutTargets.AddDefaulted_GetRef();
	Target.Actor = Context.ActivationTarget;
	Target.HitResult = Context.ActivationHitResult;
	Target.bHasHitResult = Context.bHasActivationHitResult;
}
