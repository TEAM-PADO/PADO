#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"

#include "AbilitySystemComponent.h"

bool FPDActionExecutionContext::IsAuthoritative() const
{
	return SourceAbilitySystem &&
		SourceAbilitySystem->IsOwnerActorAuthoritative();
}

AActor* FPDActionExecutionContext::ResolveScopedActor(
	EPDActionScope Scope) const
{
	return Scope == EPDActionScope::Source ? SourceActor : TargetActor;
}

UAbilitySystemComponent* FPDActionExecutionContext::ResolveScopedAbilitySystem(
	EPDActionScope Scope) const
{
	return Scope == EPDActionScope::Source
		? SourceAbilitySystem
		: TargetAbilitySystem;
}
