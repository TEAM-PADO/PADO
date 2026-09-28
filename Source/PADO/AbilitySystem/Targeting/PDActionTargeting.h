#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PDActionTargeting.generated.h"

class AActor;

/** 한 번의 행동이 관여할 대상 하나다. */
USTRUCT()
struct PADO_API FPDActionTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor;

	FHitResult HitResult;
	bool bHasHitResult = false;
};

/** Instant Targeting 한 번의 수집 결과다. */
struct PADO_API FPDActionTargetingResult
{
	TArray<FPDActionTarget> Targets;

	/**
	 * 이번 발이 멈춘 곳이다. 선을 긋는 Targeting만 채운다.
	 *
	 * 대상의 HitResult와 달리 명중 여부와 무관하게 매번 있다. 무언가에 막혔으면
	 * bBlockingHit이 true이고, 빗나갔으면 ImpactPoint가 사거리 끝이다.
	 * TraceStart와 TraceEnd는 판정 선분 그대로다.
	 */
	FHitResult ShotResult;
	bool bHasShotResult = false;
};

/** 대상 수집에 필요한 최소 입력이다. GA 구현에 의존하지 않는다. */
struct PADO_API FPDActionTargetingContext
{
	AActor* SourceActor = nullptr;
	const UObject* SourceObject = nullptr;

	/** 활성화 이벤트가 지정한 대상. Event 방식에서만 채워진다. */
	AActor* ActivationTarget = nullptr;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;
};

/**
 * 행동이 "누구를" 대상으로 삼는지 정하는 공통 데이터 기반이다.
 * 실제 수집 프로토콜은 Instant와 TraceWindow 파생 계약이 각각 소유한다.
 *
 * Definition에 인라인으로 담기는 불변 데이터이므로 실행 상태를 갖지 않는다.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PADO_API UPDActionTargeting : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const;

	/** 수집된 FPDActionTarget이 실제 충돌 HitResult를 제공하는 방식인지. */
	virtual bool ProducesHitResults() const { return false; }

	/** 수집 결과에 이번 발이 멈춘 곳(ShotResult)을 함께 채우는 방식인지. */
	virtual bool ProducesShotResult() const { return false; }

	/** 활성화가 이벤트로 대상을 넘겨줘야 하는 방식인지 알려 준다. */
	virtual bool RequiresActivationTarget() const { return false; }

};

/** 한 시점의 Context만으로 대상을 수집할 수 있는 Targeting 계약이다. */
UCLASS(Abstract)
class PADO_API UPDInstantActionTargeting : public UPDActionTargeting
{
	GENERATED_BODY()

public:

	/** 대상이 없을 수도 있다. 빗나간 행동도 정상 실행이다. */
	virtual void GatherTargets(
		const FPDActionTargetingContext& Context,
		FPDActionTargetingResult& OutResult) const
		PURE_VIRTUAL(UPDInstantActionTargeting::GatherTargets, );
};

/** NotifyState 구간의 이전/현재 Trace 선분으로 대상을 수집하는 Targeting 계약이다. */
UCLASS(Abstract)
class PADO_API UPDTraceWindowTargeting : public UPDActionTargeting
{
	GENERATED_BODY()

public:
	virtual bool ProducesHitResults() const override { return true; }

	/** 활성화 중 추적할 실제 런타임 Source를 해석한다. */
	virtual bool ResolveTraceSource(
		const FPDActionTargetingContext& Context,
		UObject*& OutTraceSource,
		FString* OutError = nullptr) const
		PURE_VIRTUAL(UPDTraceWindowTargeting::ResolveTraceSource, return false;);

	/** Source의 현재 Trace 시작점과 끝점을 월드 좌표로 구한다. */
	virtual bool GetTraceSegment(
		const UObject& TraceSource,
		FVector& OutStart,
		FVector& OutEnd) const
		PURE_VIRTUAL(UPDTraceWindowTargeting::GetTraceSegment, return false;);

	/** 이전 프레임과 현재 프레임 사이에서 이번 Tick의 대상을 수집한다. */
	virtual void GatherTraceTargets(
		const FPDActionTargetingContext& Context,
		const FVector& PreviousStart,
		const FVector& PreviousEnd,
		const FVector& CurrentStart,
		const FVector& CurrentEnd,
		TArray<FPDActionTarget>& OutTargets) const
		PURE_VIRTUAL(UPDTraceWindowTargeting::GatherTraceTargets, );

	virtual int32 GetMaxTargets() const
		PURE_VIRTUAL(UPDTraceWindowTargeting::GetMaxTargets, return 0;);
};
