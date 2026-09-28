#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CoreMinimal.h"
#include "PDAnimNotifyState_ActionSocketTraceWindow.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;

/** 현재 Action Ability의 아이템 소켓 추적 구간을 열고 갱신하고 닫는다. */
UCLASS(
	const,
	hidecategories = Object,
	collapsecategories,
	meta = (DisplayName = "PD Action Socket Trace Window"))
class PADO_API UPDAnimNotifyState_ActionSocketTraceWindow
	: public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPDAnimNotifyState_ActionSocketTraceWindow();

	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif
};
