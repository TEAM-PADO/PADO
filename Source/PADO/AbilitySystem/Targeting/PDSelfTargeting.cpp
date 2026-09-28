#include "PADO/AbilitySystem/Targeting/PDSelfTargeting.h"

#include "GameFramework/Actor.h"

void UPDSelfTargeting::GatherTargets(
	const FPDActionTargetingContext& Context,
	FPDActionTargetingResult& OutResult) const
{
	if (!IsValid(Context.SourceActor))
	{
		return;
	}

	FPDActionTarget& Target = OutResult.Targets.AddDefaulted_GetRef();
	Target.Actor = Context.SourceActor;
}
