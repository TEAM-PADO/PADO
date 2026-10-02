#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "PDZombieMoveStateTrait.generated.h"

UCLASS(meta = (DisplayName = "PD Zombie Move State"))
class PADO_API UPDZombieMoveStateTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(
		FMassEntityTemplateBuildContext& BuildContext,
		const UWorld& World) const override;
};