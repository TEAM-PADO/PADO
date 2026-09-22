#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDOverlapTargeting.generated.h"

class AActor;
class UPrimitiveComponent;

/**
 * 소스의 충돌 볼륨 안에 있는 대상을 수집한다.
 * 에디터에서 그린 볼륨이 곧 판정 범위다.
 * 대상의 Root Primitive가 볼륨과 겹치면 영역 안에 있는 것으로 본다.
 * 상호작용 Sphere 같은 보조 감지 컴포넌트의 겹침은 대상 판정에서 제외한다.
 *
 * 전방 개념이 없는 지면 함정에 쓴다.
 */
UCLASS(meta = (DisplayName = "Overlap Targeting"))
class PADO_API UPDOverlapTargeting : public UPDInstantActionTargeting
{
	GENERATED_BODY()

public:
	UPDOverlapTargeting();

	virtual bool Validate(FString& OutError) const override;

	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		TArray<FPDActionTarget>& OutTargets) const override;

	/** 판정에 쓸 볼륨이다. 비우면 소스의 Root Primitive를 쓴다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter")
	FComponentReference OverlapComponent;

	/** 수집할 Actor 종류. 비우면 겹친 모든 Actor를 수집한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter")
	TSubclassOf<AActor> TargetActorClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter",
		meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "8"))
	int32 MaxTargets = 8;

private:
	UPrimitiveComponent* ResolveOverlapComponent(AActor& SourceActor) const;
};
