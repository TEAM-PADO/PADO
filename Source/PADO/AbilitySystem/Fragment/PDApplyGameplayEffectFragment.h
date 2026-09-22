#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PADO/AbilitySystem/Struct/PDGameplayEffectRecipeStruct.h"
#include "PDApplyGameplayEffectFragment.generated.h"

/** 실행 문맥의 대상 ASC에 하나의 GameplayEffect Recipe를 적용한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Apply Gameplay Effect"))
class PADO_API UPDApplyGameplayEffectFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool DeclaresSetByCallerTag(FGameplayTag DataTag) const override;
	virtual void AppendDeclaredSetByCallerTags(
		FGameplayTagContainer& OutTags) const override;
	virtual bool SupportsDeferredExecution() const override;
	virtual bool PrepareDeferredExecution(
		const FPDActionExecutionContext& Context,
		FString& OutError) override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Effect",
		meta = (ShowOnlyInnerProperties))
	FPDGameplayEffectRecipeStruct EffectRecipe;

	/** Infinite GE를 GA 종료 시 자동 회수할 때 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	bool bTrackUntilAbilityEnds = false;

private:
	/** 원본 Ability 종료 뒤에도 적용할 수 있도록 Runtime 복제본이 캡처한 Spec이다. */
	FGameplayEffectSpecHandle DeferredEffectSpec;
};
