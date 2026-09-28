#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Engine/NetSerialization.h"
#include "GameplayAbilitySpecHandle.h"
#include "PDFireShotStruct.generated.h"

class AActor;
class UPhysicalMaterial;
class UPrimitiveComponent;

/** 한 발이 관여한 대상 하나다. 서버는 다시 추적하지 않고 이 값으로 결과를 적용한다. */
USTRUCT()
struct PADO_API FPDFireShotHitStruct
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY()
	FName BoneName;

	UPROPERTY()
	FVector_NetQuantize ImpactPoint = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal ImpactNormal = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<UPhysicalMaterial> PhysMaterial;

	/** Targeting이 충돌 결과를 줬는가. 범위 수집처럼 대상만 주는 방식이면 false다. */
	UPROPERTY()
	bool bHasHitResult = false;

	static FPDFireShotHitStruct Make(AActor* InActor, const FHitResult* Hit);
	FHitResult ToHitResult(const FVector& TraceStart, const FVector& TraceEnd) const;
};

/**
 * 쏜 머신이 판정한 한 발이다. 서버는 이것을 그대로 적용하고 관찰자에게 전달한다.
 *
 * 발사 시각은 싣지 않는다. 받는 쪽이 쓸 곳이 없다. 명중 인정 한계는 서버가 잰
 * 핑으로 판단하고, 관찰자는 도착하는 대로 보여 준다.
 */
USTRUCT()
struct PADO_API FPDFireShotStruct
{
	GENERATED_BODY()

	/**
	 * Ability Spec 안의 발 번호다. 1부터 증가한다. 탄약 예측 보정과 몇 발마다
	 * 재생하는 연출이 쓰며, 모든 머신이 같은 값을 본다.
	 */
	UPROPERTY()
	int32 ShotIndex = 0;

	/** 판정 선분의 시작점이다. */
	UPROPERTY()
	FVector_NetQuantize TraceStart = FVector::ZeroVector;

	/** 이번 발이 멈춘 곳이다. 막혔으면 표면, 빗나갔으면 사거리 끝이다. */
	UPROPERTY()
	FVector_NetQuantize ShotEnd = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal ImpactNormal = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<UPhysicalMaterial> PhysMaterial;

	UPROPERTY()
	bool bBlockingHit = false;

	/** Targeting이 이번 발이 멈춘 곳을 만들었는가. 선을 긋는 방식만 만든다. */
	UPROPERTY()
	bool bHasShotResult = false;

	/** 관여한 대상이다. 수집 순서 그대로이며, 충돌 결과가 있는 첫 항목이 첫 명중이다. */
	UPROPERTY()
	TArray<FPDFireShotHitStruct> Hits;

	void SetShotResult(const FHitResult& ShotResult);
	FHitResult MakeShotResult() const;
};

/** 한 번에 보내는 발 묶음이다. 한 묶음은 한 Ability Spec에서만 나온다. */
USTRUCT()
struct PADO_API FPDFireShotBatchStruct
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilityHandle;

	/**
	 * 발을 쏜 Ability Source다. 무기 식별자이며, GA 인스턴스가 없는 관찰자는
	 * 여기서 Definition을 얻는다.
	 */
	UPROPERTY()
	TObjectPtr<UObject> SourceObject;

	UPROPERTY()
	TArray<FPDFireShotStruct> Shots;
};
