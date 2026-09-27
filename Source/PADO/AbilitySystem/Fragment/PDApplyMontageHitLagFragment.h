#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PADO/AbilitySystem/Struct/PDMontageHitLagConfigStruct.h"
#include "PDApplyMontageHitLagFragment.generated.h"

/** 명중이 확정됐을 때 소스의 현재 공격 몽타주를 잠깐 느리게 만든다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Apply Montage Hit Lag"))
class PADO_API UPDApplyMontageHitLagFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	UPDApplyMontageHitLagFragment();

	virtual bool Validate(FString& OutError) const override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (ShowOnlyInnerProperties))
	FPDMontageHitLagConfigStruct HitLag;
};
