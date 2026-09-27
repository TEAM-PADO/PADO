#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PADO/AbilitySystem/Interface/PDAimStateProvider.h"
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
	virtual bool ProducesShotResult() const override { return true; }
	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		FPDActionTargetingResult& OutResult) const override;

	/** 아래 맵에 없는 조준 단계는 이 값을 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace")
	EPDAimTraceOrigin TraceOrigin = EPDAimTraceOrigin::SourceViewPoint;

	/**
	 * 조준 단계별 시작점 재정의다.
	 * 비워 두면 언제나 TraceOrigin을 쓴다. 카메라 기준으로만 나가야 하는
	 * 스킬은 비워 두고, 총기는 견착에만 ItemSocket을 지정하는 식으로 쓴다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trace")
	TMap<EPDAimState, EPDAimTraceOrigin> OriginByAimState;

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
	EPDAimTraceOrigin ResolveOriginForAimState(
		const FPDActionTargetingContext& Context) const;

	/** 화면 중앙이 가리키는 지점이다. 2단계 판정의 목표점이 된다. */
	bool ResolveAimPoint(
		const FPDActionTargetingContext& Context,
		const UWorld& World,
		const FCollisionQueryParams& QueryParams,
		FVector& OutViewStart,
		FRotator& OutAimRotation,
		FVector& OutAimPoint) const;

	bool ResolveTrace(
		const FPDActionTargetingContext& Context,
		const UWorld& World,
		const FCollisionQueryParams& QueryParams,
		FVector& OutStart,
		FVector& OutEnd) const;
};
