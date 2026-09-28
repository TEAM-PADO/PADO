#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDAbilityTask_ActionTraceWindow.generated.h"

class AActor;
class UPDGA_ActionRuntimeBase;

/**
 * 한 Action Ability가 재생되는 동안 여러 Trace NotifyState 구간을 처리한다.
 * 구간별 이전 선분과 이미 명중한 Actor를 이 Task가 독립적으로 소유한다.
 */
UCLASS()
class PADO_API UPDAbilityTask_ActionTraceWindow : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UPDAbilityTask_ActionTraceWindow* Create(
		UPDGA_ActionRuntimeBase* OwningAbility,
		const UPDTraceWindowTargeting* InTargeting);

	virtual void Activate() override;

	void BeginTraceWindow();
	void TickTraceWindow();
	void EndTraceWindow();

protected:
	virtual void OnDestroy(bool bAbilityEnded) override;

private:
	bool TraceToCurrentSegment();
	void ResetWindow();

	UPROPERTY(Transient)
	TObjectPtr<UPDGA_ActionRuntimeBase> RuntimeAbility;

	UPROPERTY(Transient)
	TObjectPtr<UPDTraceWindowTargeting> Targeting;

	UPROPERTY(Transient)
	TObjectPtr<UObject> TraceSource;

	TSet<TWeakObjectPtr<AActor>> HitActors;
	FVector PreviousStart = FVector::ZeroVector;
	FVector PreviousEnd = FVector::ZeroVector;
	bool bWindowActive = false;
};
