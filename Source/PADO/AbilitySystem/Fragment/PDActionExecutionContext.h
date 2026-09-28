#pragma once

#include "CoreMinimal.h"
#include "PDActionExecutionContext.generated.h"

class AActor;
class UAbilitySystemComponent;
class UPDActionFragment;
class UPDGA_Base;

/** 이번 Hook 실행에서 어떤 Fragment를 돌릴지 정한다. */
enum class EPDActionExecutionScope : uint8
{
	/** 서버 실행이다. 모든 Fragment를 실행한다. */
	Authority,

	/**
	 * 소유 클라이언트가 서버 확정 전에 미리 실행한다. 서버가 거부해도 되돌릴
	 * 필요가 없는 Fragment(SupportsLocalPrediction)만 실행한다.
	 */
	Predicting,

	/**
	 * 연출 Fragment만 이 머신에서 복제 없이 실행한다. Fire Action에서 쏜 머신과
	 * 관찰자가 쓴다. 판정을 쏜 머신이 하므로 탄착 연출까지 각자 재생한다.
	 */
	PresentationOnly,

	/** 결과 Fragment만 실행한다. 연출은 각 머신이 따로 한다. Fire Action의 서버가 쓴다. */
	ResultsOnly
};

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

	/**
	 * 이번 발이 멈춘 곳이다. OnExecuteStart에서, 선을 긋는 Instant Targeting일
	 * 때만 채워진다. 트레이서와 탄착 연출이 쓴다.
	 *
	 * HitResult와 따로 둔다. HitResult는 대상에게 맞은 결과라서 이것을 넣으면
	 * HitResult를 명중 위치로 읽는 기존 Fragment들이 빗나간 탄에도 반응한다.
	 */
	FHitResult ShotResult;
	bool bHasShotResult = false;

	/**
	 * 어떤 Fragment를 실행할지 정한다.
	 *
	 * 예측 실행에서는 게임 상태를 바꾸면 안 된다. 서버가 같은 일을 다시 하고,
	 * 거부해도 되돌릴 방법이 없다. 연출처럼 되돌릴 필요가 없는 것만 예측한다.
	 */
	EPDActionExecutionScope ExecutionScope = EPDActionExecutionScope::Authority;

	/**
	 * Fire Action의 발 번호다. 1부터 증가하고 모든 머신이 같은 값을 본다.
	 * 발 단위가 아닌 실행에서는 INDEX_NONE이다.
	 */
	int32 ShotIndex = INDEX_NONE;

	bool IsAuthoritative() const;
	bool IsPredicting() const;

	/** 서버 실행이거나 소유 클라이언트의 예측 실행인가. */
	bool IsAuthoritativeOrPredicting() const;

	/** 이 머신에서 연출을 재생해도 되는가. 연출 전용 실행이면 권한과 무관하다. */
	bool CanPlayPresentation() const;

	/** 실행 범위가 이 Fragment를 허용하는가. Hook 실행이 한 곳에서 거른다. */
	bool AllowsFragment(const UPDActionFragment& Fragment) const;

	AActor* ResolveScopedActor(EPDActionScope Scope) const;
	UAbilitySystemComponent* ResolveScopedAbilitySystem(EPDActionScope Scope) const;
};
