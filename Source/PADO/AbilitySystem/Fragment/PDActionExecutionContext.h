#pragma once

#include "CoreMinimal.h"
#include "PDActionExecutionContext.generated.h"

class AActor;
class UAbilitySystemComponent;
class UPDGA_Base;

/** Fragment가 결과를 누구에게 적용할지 정한다. */
UENUM(BlueprintType)
enum class EPDActionScope : uint8
{
	/** 이 Hook이 지목한 대상. 대상이 없는 시점에서는 소스와 같다. */
	Target,

	/** 행동을 수행한 소스 자신. 흡혈·반동·자기 버프에 쓴다. */
	Source
};

/** Action Hook의 Fragment들이 공유하는 한 번의 실행 문맥이다. */
struct PADO_API FPDActionExecutionContext
{
	UPDGA_Base* Ability = nullptr;
	UAbilitySystemComponent* SourceAbilitySystem = nullptr;
	UAbilitySystemComponent* TargetAbilitySystem = nullptr;
	AActor* SourceActor = nullptr;
	AActor* TargetActor = nullptr;
	/** Ability나 원본 아이템이 사라진 뒤에도 EffectContext에 유지할 논리적 Source다. */
	UObject* EffectSourceObject = nullptr;
	/** 입력 충전 비율이다. 충전을 사용하지 않는 Action은 항상 1이다. */
	float InputChargeAlpha = 1.0f;
	FHitResult HitResult;
	bool bHasHitResult = false;

	bool IsAuthoritative() const;

	AActor* ResolveScopedActor(EPDActionScope Scope) const;
	UAbilitySystemComponent* ResolveScopedAbilitySystem(EPDActionScope Scope) const;
};
