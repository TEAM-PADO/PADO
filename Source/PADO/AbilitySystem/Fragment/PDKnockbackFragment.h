#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PDKnockbackFragment.generated.h"

UENUM(BlueprintType)
enum class EPDKnockbackDirectionMode : uint8
{
	SourceToTarget,
	SourceForward,
	InverseHitNormal
};

/** 실행 문맥의 Target에 선택적으로 물리 넉백을 적용한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Knockback"))
class PADO_API UPDKnockbackFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool SupportsDeferredExecution() const override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float HorizontalSpeed = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float VerticalSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	EPDKnockbackDirectionMode DirectionMode =
		EPDKnockbackDirectionMode::SourceToTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	bool bOverrideHorizontalVelocity = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	bool bOverrideVerticalVelocity = true;

private:
	bool BuildRequest(
		const FPDActionExecutionContext& Context,
		struct FPDKnockbackRequest& OutRequest) const;
	FVector ResolveDirection(const FPDActionExecutionContext& Context) const;
};
