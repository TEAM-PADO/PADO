#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PDItemSocketTrailTargeting.generated.h"

class UStaticMeshComponent;

/**
 * 아이템 메시의 두 소켓이 NotifyState 구간 동안 지나간 부피에서 대상을 수집한다.
 * 이전 프레임 위치와 중복 명중 상태는 런타임 AbilityTask가 소유한다.
 */
UCLASS(meta = (DisplayName = "Item Socket Trail Targeting"))
class PADO_API UPDItemSocketTrailTargeting : public UPDTraceWindowTargeting
{
	GENERATED_BODY()

public:
	UPDItemSocketTrailTargeting();

	virtual bool Validate(FString& OutError) const override;

	virtual bool ResolveTraceSource(
		const FPDActionTargetingContext& Context,
		UObject*& OutTraceSource,
		FString* OutError = nullptr) const override;

	virtual bool GetTraceSegment(
		const UObject& TraceSource,
		FVector& OutStart,
		FVector& OutEnd) const override;

	virtual void GatherTraceTargets(
		const FPDActionTargetingContext& Context,
		const FVector& PreviousStart,
		const FVector& PreviousEnd,
		const FVector& CurrentStart,
		const FVector& CurrentEnd,
		TArray<FPDActionTarget>& OutTargets) const override;

	virtual int32 GetMaxTargets() const override { return MaxTargets; }

	/** 아이템 자루 쪽 Trace 기준 소켓이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName StartSocketName = TEXT("TraceStart");

	/** 아이템 끝 쪽 Trace 기준 소켓이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName EndSocketName = TEXT("TraceEnd");

	/** 각 샘플 지점이 프레임 사이를 이동할 때 사용하는 Sphere 반경이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Trace",
		meta = (ClampMin = "0.1", UIMin = "0.1", Units = "cm"))
	float TraceRadius = 8.0f;

	/** Start-End 소켓 사이에 균등하게 배치할 Trace 개수다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Trace",
		meta = (ClampMin = "2", ClampMax = "16", UIMin = "2", UIMax = "8"))
	int32 SamplesAlongItem = 4;

	/** NotifyState 한 구간에서 명중시킬 수 있는 최대 대상 수다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter",
		meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "8"))
	int32 MaxTargets = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Filter")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	/** 벽 너머의 대상을 제외할지 결정한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstruction")
	bool bRequireUnobstructedPath = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Obstruction",
		meta = (EditCondition = "bRequireUnobstructedPath"))
	TEnumAsByte<ETraceTypeQuery> ObstructionTraceChannel;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (DisplayName = "Draw Debug Trace"))
	bool bDrawDebugTrace = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (EditCondition = "bDrawDebugTrace", ClampMin = "0.0", Units = "s"))
	float DebugDrawDuration = 0.1f;

private:
	bool IsPathUnobstructed(
		const FPDActionTargetingContext& Context,
		const FVector& TraceOrigin,
		const AActor& TargetActor) const;
};
