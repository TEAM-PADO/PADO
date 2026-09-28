#include "PADO/AbilitySystem/Task/PDAbilityTask_ActionTraceWindow.h"

#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"

UPDAbilityTask_ActionTraceWindow* UPDAbilityTask_ActionTraceWindow::Create(
	UPDGA_ActionRuntimeBase* OwningAbility,
	const UPDTraceWindowTargeting* InTargeting)
{
	UPDAbilityTask_ActionTraceWindow* Task =
		NewAbilityTask<UPDAbilityTask_ActionTraceWindow>(OwningAbility);
	if (Task)
	{
		Task->RuntimeAbility = OwningAbility;
		Task->Targeting = const_cast<UPDTraceWindowTargeting*>(InTargeting);
	}
	return Task;
}

void UPDAbilityTask_ActionTraceWindow::Activate()
{
	if (!IsValid(RuntimeAbility) || !IsValid(Targeting))
	{
		EndTask();
	}
}

void UPDAbilityTask_ActionTraceWindow::BeginTraceWindow()
{
	if (bWindowActive || !IsValid(RuntimeAbility) ||
		RuntimeAbility->IsFinishingAction() || !IsValid(Targeting))
	{
		return;
	}

	const FPDActionTargetingContext Context =
		RuntimeAbility->BuildTargetingContext();
	UObject* ResolvedSource = nullptr;
	if (!Targeting->ResolveTraceSource(Context, ResolvedSource) ||
		!ResolvedSource ||
		!Targeting->GetTraceSegment(
			*ResolvedSource, PreviousStart, PreviousEnd) ||
		!RuntimeAbility->BeginExecutionWindow())
	{
		return;
	}

	TraceSource = ResolvedSource;
	HitActors.Reset();
	bWindowActive = true;

	// 구간 시작 프레임에 이미 겹쳐 있는 대상도 놓치지 않는다.
	TraceToCurrentSegment();
}

void UPDAbilityTask_ActionTraceWindow::TickTraceWindow()
{
	if (bWindowActive && IsValid(RuntimeAbility) &&
		!RuntimeAbility->IsFinishingAction())
	{
		TraceToCurrentSegment();
	}
}

void UPDAbilityTask_ActionTraceWindow::EndTraceWindow()
{
	if (!bWindowActive)
	{
		return;
	}

	// Montage 중단 시에도 NotifyEnd가 Interrupted Delegate보다 먼저 호출된다.
	// 취소되는 공격이 여기서 새 판정을 만들지 않도록 구간 상태만 정리한다.
	ResetWindow();
}

void UPDAbilityTask_ActionTraceWindow::OnDestroy(bool bAbilityEnded)
{
	ResetWindow();
	RuntimeAbility = nullptr;
	Targeting = nullptr;
	Super::OnDestroy(bAbilityEnded);
}

bool UPDAbilityTask_ActionTraceWindow::TraceToCurrentSegment()
{
	if (!bWindowActive || !IsValid(RuntimeAbility) ||
		!IsValid(Targeting) || !IsValid(TraceSource) ||
		HitActors.Num() >= Targeting->GetMaxTargets())
	{
		return false;
	}

	FVector CurrentStart;
	FVector CurrentEnd;
	if (!Targeting->GetTraceSegment(
		*TraceSource, CurrentStart, CurrentEnd))
	{
		return false;
	}

	const FPDActionTargetingContext Context =
		RuntimeAbility->BuildTargetingContext();
	TArray<FPDActionTarget> GatheredTargets;
	Targeting->GatherTraceTargets(
		Context,
		PreviousStart,
		PreviousEnd,
		CurrentStart,
		CurrentEnd,
		GatheredTargets);

	TArray<FPDActionTarget> NewTargets;
	for (const FPDActionTarget& Target : GatheredTargets)
	{
		if (HitActors.Num() >= Targeting->GetMaxTargets())
		{
			break;
		}

		const TWeakObjectPtr<AActor> TargetKey(Target.Actor.Get());
		if (!TargetKey.IsValid() || HitActors.Contains(TargetKey))
		{
			continue;
		}

		HitActors.Add(TargetKey);
		NewTargets.Add(Target);
	}

	if (!NewTargets.IsEmpty())
	{
		RuntimeAbility->ExecuteTargets(NewTargets);
	}

	PreviousStart = CurrentStart;
	PreviousEnd = CurrentEnd;
	return true;
}

void UPDAbilityTask_ActionTraceWindow::ResetWindow()
{
	bWindowActive = false;
	TraceSource = nullptr;
	HitActors.Reset();
	PreviousStart = FVector::ZeroVector;
	PreviousEnd = FVector::ZeroVector;
}
