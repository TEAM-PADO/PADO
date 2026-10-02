#include "PDZombieMoveStateTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "PDZombieMoveStateFragment.h"

void UPDZombieMoveStateTrait::BuildTemplate(
	FMassEntityTemplateBuildContext& BuildContext,
	const UWorld& World) const
{
	BuildContext.AddFragment<FPDZombieMoveStateFragment>();
}