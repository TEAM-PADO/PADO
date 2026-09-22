#include "PADO/AbilitySystem/Targeting/PDSelfTargeting.h"

#include "GameFramework/Actor.h"

void UPDSelfTargeting::GatherTargets(
	const FPDActionTargetingContext& Context,
	TArray<FPDActionTarget>& OutTargets) const
{
	if (!IsValid(Context.SourceActor))
	{
		return;
	}

	FPDActionTarget& Target = OutTargets.AddDefaulted_GetRef();
	Target.Actor = Context.SourceActor;
}
