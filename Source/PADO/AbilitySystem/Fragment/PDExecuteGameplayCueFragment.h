#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PDExecuteGameplayCueFragment.generated.h"

/** GameplayCue 이펙트가 어느 쪽을 보고 재생될지 정한다. */
UENUM(BlueprintType)
enum class EPDGameplayCueDirectionMode : uint8
{
	/**
	 * 방향을 지정하지 않는다. 기준 HitResult가 있으면 그 Normal을, 없으면 Notify
	 * 기본 배치를 따른다. 이번 발이 멈춘 곳에서 재생하면 그 결과가 기준이다.
	 */
	FromContext,

	/** 소스 Actor의 정면이다. 손에 든 아이템에서 앞으로 뻗는 이펙트에 쓴다. */
	SourceForward,

	/** 소스 Pawn의 시선 방향이다. 상하 조준까지 따라간다. */
	SourceAim,

	/** 소스에서 대상으로 향하는 방향이다. */
	SourceToTarget,

	/** HitResult Normal을 파고드는 방향이다. */
	InverseHitNormal
};

/** 실행 문맥의 위치·표면·SourceObject를 담아 일회성 GameplayCue를 실행한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Execute Gameplay Cue"))
class PADO_API UPDExecuteGameplayCueFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool SupportsDeferredExecution() const override;
	virtual bool SupportsLocalPrediction() const override;
	virtual bool RequiresShotResult() const override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	/**
	 * 소유 클라이언트가 서버 확정을 기다리지 않고 미리 재생한다.
	 *
	 * 총구 화염이나 발사음처럼 자기가 한 행동의 즉각적인 연출에 켠다. 끄면
	 * 서버가 확정한 뒤 멀티캐스트로 오므로 자기 화면도 왕복만큼 늦는다.
	 *
	 * 서버가 발사를 거부하면 이미 재생된 연출은 되돌릴 수 없다. 한 프레임짜리
	 * 연출만 켜고, 탄착처럼 서버가 정한 위치가 필요한 것은 켜지 않는다.
	 * 그래서 Target Scope에는 쓸 수 없다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	bool bPredictOnOwningClient = false;

	/**
	 * 이번 발이 멈춘 곳에서 재생한다. 트레이서와 탄착 연출에 켠다.
	 *
	 * Cue의 HitResult가 이번 발의 결과가 된다. 막혔으면 bBlockingHit이 true이고,
	 * 빗나갔으면 ImpactPoint가 사거리 끝이다. TraceStart와 TraceEnd도 함께 간다.
	 * 탄의 출발점은 GCN이 재생하는 쪽의 총구 소켓에서 직접 구한다. 서버가 구한
	 * 소켓 위치는 다른 클라이언트의 애니메이션 포즈와 다를 수 있다.
	 *
	 * OnExecuteStart에만 둘 수 있고, Aim Line Trace처럼 결과를 만드는 Targeting이
	 * 필요하다. 위치를 소켓으로 덮는 ItemSocketName과는 함께 쓸 수 없다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	bool bPlayAtShotEnd = false;

	/**
	 * 이번 발이 무언가에 막혔을 때만 재생한다. 허공으로 빗나간 탄의 탄착 연출을
	 * 거른다. 대상에 맞아 멈춘 것도 막힌 것이다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (EditCondition = "bPlayAtShotEnd"))
	bool bOnlyWhenShotBlocked = false;

	/**
	 * Cue Notify가 이펙트를 어느 방향으로 회전시킬지 정한다.
	 * 엔진은 이 방향을 CueParameters.Normal로 받아 +X가 그쪽을 보도록 회전시키므로,
	 * Niagara System도 로컬 +X를 향해 뿜도록 만들어져 있어야 한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	EPDGameplayCueDirectionMode DirectionMode =
		EPDGameplayCueDirectionMode::FromContext;

	/** true면 방향의 상하 성분을 지워 수평으로 눕힌다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (EditCondition =
			"DirectionMode != EPDGameplayCueDirectionMode::FromContext"))
	bool bFlattenDirection = false;

	/**
	/**
	 * 구한 방향을 기준으로 한 로컬 회전이다. 월드 축이 아니라 그 방향의 프레임에서 돈다.
	 * Yaw 180이나 Pitch 180이면 정반대를 보고, Pitch 90이면 위를 본다.
	 * Roll은 방향 벡터를 바꾸지 못해 효과가 없다. Cue는 회전이 아니라 방향만 실어 나른다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (EditCondition =
			"DirectionMode != EPDGameplayCueDirectionMode::FromContext"))
	FRotator DirectionOffset = FRotator::ZeroRotator;

	/**
	 * 비워 두지 않으면 원본 아이템 메시의 이 소켓 위치에서 Cue를 재생한다.
	 * 토치 주둥이처럼 아이템 원점이 아닌 곳에서 이펙트가 나가야 할 때 쓴다.
	 * HitResult가 있어도 위치만 소켓으로 바꾸고, 방향과 표면 정보는 그대로 둔다.
	 * 소켓을 찾지 못하면 경고를 남기고 기존 위치에서 재생한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Settings",
		meta = (EditCondition = "!bPlayAtShotEnd"))
	FName ItemSocketName = NAME_None;

	/** 설정한 모드와 오프셋으로 이펙트가 바라볼 방향을 구한다. 못 구하면 0 벡터다. */
	FVector ResolveDirection(const FPDActionExecutionContext& Context) const;

	/** EffectCauser가 아이템이면 ItemSocketName의 월드 위치를 구한다. */
	bool ResolveItemSocketLocation(
		const AActor* EffectCauser,
		FVector& OutLocation) const;

private:
	/** 이 Cue가 위치와 방향의 기준으로 삼을 HitResult다. 없으면 nullptr다. */
	const FHitResult* ResolveCueHit(const FPDActionExecutionContext& Context) const;
};
