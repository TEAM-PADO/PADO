#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDSweepTargeting.generated.h"

UENUM(BlueprintType)
enum class EPDSweepShape : uint8
{
	Sphere UMETA(DisplayName = "Sphere"),
	Box UMETA(DisplayName = "Box")
};

/** 소스 전방을 Sphere 또는 Box로 Sweep해 대상을 수집한다. */
UCLASS(meta = (DisplayName = "Sweep Targeting"))
class PADO_API UPDSweepTargeting : public UPDInstantActionTargeting
{
	GENERATED_BODY()

public:
	UPDSweepTargeting();

	virtual bool Validate(FString& OutError) const override;
	virtual bool ProducesHitResults() const override { return true; }

	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		FPDActionTargetingResult& OutResult) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shape")
	EPDSweepShape Shape = EPDSweepShape::Sphere;

	/** Sweep 시작점부터 전방으로 검사할 거리다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float SweepDistance = 180.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (
			EditCondition = "Shape == EPDSweepShape::Sphere",
			EditConditionHides,
			ClampMin = "0.0",
			UIMin = "0.0",
			Units = "cm"))
	float SweepRadius = 60.0f;

	/** Box의 로컬 X/Y/Z 반경이다. X축이 Sweep 진행 방향을 향한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (
			EditCondition = "Shape == EPDSweepShape::Box",
			EditConditionHides,
			ClampMin = "0.1",
			UIMin = "0.1",
			Units = "cm"))
	FVector BoxHalfExtent = FVector(45.0f, 60.0f, 45.0f);

	/** 진행 방향을 기준으로 Box에 더할 회전이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (
			EditCondition = "Shape == EPDSweepShape::Box",
			EditConditionHides))
	FRotator BoxRotationOffset = FRotator::ZeroRotator;

	/** Avatar 원점에서 전방으로 떨어진 Sweep 시작 위치다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float StartForwardOffset = 45.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (Units = "cm"))
	float HeightOffset = 45.0f;

	/** 한 번에 수집할 최대 대상 수다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter",
		meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "8"))
	int32 MaxTargets = 1;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	/** 벽 너머의 대상을 제외할지 결정한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Obstruction")
	bool bRequireUnobstructedPath = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Obstruction",
		meta = (EditCondition = "bRequireUnobstructedPath"))
	TEnumAsByte<ETraceTypeQuery> ObstructionTraceChannel;

	/** 개발용. Sweep 범위와 수집된 대상을 월드에 그린다. Shipping에서는 그리지 않는다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (DisplayName = "Draw Debug Sweep"))
	bool bDrawDebugSweep = false;

	/** 디버그 도형이 화면에 남는 시간(초). */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (EditCondition = "bDrawDebugSweep", ClampMin = "0.0", Units = "s"))
	float DebugDrawDuration = 1.0f;

private:
	void DrawDebugSweep(
		const UWorld& World,
		const FVector& Start,
		const FVector& End,
		const FQuat& ShapeRotation,
		const TArray<FPDActionTarget>& Targets) const;

	bool IsPathUnobstructed(
		const FPDActionTargetingContext& Context,
		const AActor& TargetActor) const;
};
