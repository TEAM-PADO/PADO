#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDAimLineTraceTargeting.generated.h"

UENUM(BlueprintType)
enum class EPDAimTraceOrigin : uint8
{
	SourceActor,
	SourceViewPoint,
	ItemSocket
};

/** Pawn 조준 방향으로 선형 판정을 수행하는 PADO 총기용 Targeting이다. */
UCLASS(meta = (DisplayName = "Aim Line Trace Targeting"))
class PADO_API UPDAimLineTraceTargeting : public UPDInstantActionTargeting
{
	GENERATED_BODY()

public:
	UPDAimLineTraceTargeting();

	virtual bool Validate(FString& OutError) const override;
	virtual bool ProducesHitResults() const override { return true; }
	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		TArray<FPDActionTarget>& OutTargets) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace")
	EPDAimTraceOrigin TraceOrigin = EPDAimTraceOrigin::SourceViewPoint;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Trace",
		meta = (EditCondition = "TraceOrigin == EPDAimTraceOrigin::ItemSocket"))
	FName ItemSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace")
	FVector OriginOffset = FVector::ZeroVector;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Trace",
		meta = (ClampMin = "1.0", Units = "cm"))
	float TraceDistance = 10000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Filter")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Filter")
	TSubclassOf<AActor> TargetActorClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter",
		meta = (ClampMin = "1", ClampMax = "32"))
	int32 MaxTargets = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstruction")
	bool bRequireUnobstructedPath = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Obstruction",
		meta = (EditCondition = "bRequireUnobstructedPath"))
	TEnumAsByte<ETraceTypeQuery> ObstructionTraceChannel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugTrace = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (EditCondition = "bDrawDebugTrace", ClampMin = "0.0", Units = "s"))
	float DebugDrawDuration = 0.5f;

private:
	bool ResolveTrace(
		const FPDActionTargetingContext& Context,
		FVector& OutStart,
		FVector& OutEnd) const;
};
