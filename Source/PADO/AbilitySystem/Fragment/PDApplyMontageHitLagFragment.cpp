#include "PADO/AbilitySystem/Fragment/PDApplyMontageHitLagFragment.h"

#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"

UPDApplyMontageHitLagFragment::UPDApplyMontageHitLagFragment()
{
	ApplicationScope = EPDActionScope::Source;
	bRequired = false;
}

bool UPDApplyMontageHitLagFragment::Validate(FString& OutError) const
{
	if (ApplicationScope != EPDActionScope::Source)
	{
		OutError = TEXT("Montage Hit Lag Fragment의 ApplicationScope는 Source여야 합니다.");
		return false;
	}

	return HitLag.Validate(OutError);
}

bool UPDApplyMontageHitLagFragment::CanExecute(
	const FPDActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	const UPDGA_ActionRuntimeBase* ActionAbility =
		Cast<UPDGA_ActionRuntimeBase>(Context.Ability);
	if (!Context.IsAuthoritative() || !Context.bHasHitResult || !ActionAbility ||
		!ActionAbility->CanApplyMontageHitLag())
	{
		OutError = TEXT("역경직을 적용할 명중 문맥, 서버 권한 또는 재생 중인 Action Montage가 없습니다.");
		return false;
	}

	return true;
}

bool UPDApplyMontageHitLagFragment::Execute(
	const FPDActionExecutionContext& Context) const
{
	UPDGA_ActionRuntimeBase* ActionAbility =
		Cast<UPDGA_ActionRuntimeBase>(Context.Ability);
	return ActionAbility && ActionAbility->ApplyMontageHitLag(HitLag);
}
