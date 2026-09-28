#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"

#include "AbilitySystemComponent.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"

bool FPDActionExecutionContext::IsAuthoritative() const
{
	return SourceAbilitySystem &&
		SourceAbilitySystem->IsOwnerActorAuthoritative();
}

bool FPDActionExecutionContext::IsPredicting() const
{
	return ExecutionScope == EPDActionExecutionScope::Predicting;
}

bool FPDActionExecutionContext::IsAuthoritativeOrPredicting() const
{
	return IsPredicting() || IsAuthoritative();
}

bool FPDActionExecutionContext::CanPlayPresentation() const
{
	return ExecutionScope == EPDActionExecutionScope::PresentationOnly ||
		IsAuthoritativeOrPredicting();
}

bool FPDActionExecutionContext::AllowsFragment(
	const UPDActionFragment& Fragment) const
{
	switch (ExecutionScope)
	{
	case EPDActionExecutionScope::Predicting:
		return Fragment.SupportsLocalPrediction();

	case EPDActionExecutionScope::PresentationOnly:
		return Fragment.IsPresentationOnly();

	case EPDActionExecutionScope::ResultsOnly:
		return !Fragment.IsPresentationOnly();

	case EPDActionExecutionScope::Authority:
	default:
		return true;
	}
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
