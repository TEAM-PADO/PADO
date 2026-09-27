#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDSelfTargeting.generated.h"

/** 행동을 수행하는 Avatar 자신을 대상으로 삼는다. */
UCLASS(meta = (DisplayName = "Self Targeting"))
class PADO_API UPDSelfTargeting : public UPDInstantActionTargeting
{
	GENERATED_BODY()

public:
	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		FPDActionTargetingResult& OutResult) const override;
};
