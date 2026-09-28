#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"

bool UPDActionFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	return true;
}

bool UPDActionFragment::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return false;
}

void UPDActionFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
}

bool UPDActionFragment::SupportsLocalPrediction() const
{
	return false;
}

bool UPDActionFragment::IsPresentationOnly() const
{
	return false;
}

bool UPDActionFragment::RequiresShotResult() const
{
	return false;
}

bool UPDActionFragment::CanActivateWithPredictedState(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	return true;
}

bool UPDActionFragment::SupportsDeferredExecution() const
{
	return false;
}

bool UPDActionFragment::PrepareDeferredExecution(
	const FPDActionExecutionContext& Context,
	FString& OutError)
{
	OutError.Reset();
	if (!SupportsDeferredExecution())
	{
		OutError = TEXT("이 Fragment는 지연 실행을 지원하지 않습니다.");
		return false;
	}
	return true;
}

bool UPDActionFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	return true;
}
