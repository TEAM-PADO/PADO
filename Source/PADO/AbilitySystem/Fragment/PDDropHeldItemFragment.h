#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PDDropHeldItemFragment.generated.h"

UENUM(BlueprintType)
enum class EPDDropItemImpulseDirectionMode : uint8
{
	SourceToTarget,
	SourceForward,
	InverseHitNormal
};

/** 실행 문맥의 대상이 들고 있는 아이템을 즉시 드롭시킨다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Drop Held Item"))
class PADO_API UPDDropHeldItemFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool SupportsDeferredExecution() const override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

	/** 켜면 기존 드롭 Impulse에 아래 값을 추가한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop|Impulse")
	bool bApplyImpulse = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Drop|Impulse",
		meta = (EditCondition = "bApplyImpulse", ClampMin = "0.0"))
	float HorizontalImpulse = 600.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Drop|Impulse",
		meta = (EditCondition = "bApplyImpulse", ClampMin = "0.0"))
	float VerticalImpulse = 200.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Drop|Impulse",
		meta = (EditCondition = "bApplyImpulse"))
	EPDDropItemImpulseDirectionMode DirectionMode =
		EPDDropItemImpulseDirectionMode::SourceToTarget;

private:
	FVector BuildAdditionalImpulse(
		const FPDActionExecutionContext& Context) const;
	FVector ResolveHorizontalDirection(
		const FPDActionExecutionContext& Context) const;
};
