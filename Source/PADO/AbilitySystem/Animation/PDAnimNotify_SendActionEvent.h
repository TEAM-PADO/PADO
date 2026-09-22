#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "PDAnimNotify_SendActionEvent.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;

/**
 * 몽타주의 실제 결과 프레임에서 Action Gameplay Event를 보낸다.
 * 소리나 Trail 같은 순수 연출은 이 Notify와 분리한다.
 */
UCLASS(
	const,
	hidecategories = Object,
	collapsecategories,
	meta = (DisplayName = "PD Send Action Event"))
class PADO_API UPDAnimNotify_SendActionEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPDAnimNotify_SendActionEvent();

	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif

protected:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Ability|Action",
		meta = (Categories = "GameplayEvent.Action"))
	FGameplayTag ActionEventTag;
};
