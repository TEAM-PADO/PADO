#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PDProjectileConfigStruct.generated.h"

class APDActionProjectile;
class UStaticMesh;

/** 투사체가 폭발하는 시점을 정한다. */
UENUM(BlueprintType)
enum class EPDProjectileDetonationMode : uint8
{
	/** 처음 발생한 Blocking Hit에서 즉시 폭발한다. */
	OnImpact UMETA(DisplayName = "On Impact"),

	/** 지형과 Pawn에 튕기며 설정한 퓨즈가 끝났을 때 폭발한다. */
	OnFuse UMETA(DisplayName = "On Fuse"),

	/** Blocking Hit 또는 퓨즈 중 먼저 발생한 시점에 폭발한다. */
	OnImpactOrFuse UMETA(DisplayName = "On Impact Or Fuse")
};

/** Throw Projectile Fragment가 투사체를 생성하고 날리는 데 필요한 설정이다. */
USTRUCT(BlueprintType)
struct PADO_API FPDProjectileLaunchConfigStruct
{
	GENERATED_BODY()

	/** 비어 있으면 실행할 수 없다. BP 파생 클래스로 외형이나 추가 연출을 확장할 수 있다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<APDActionProjectile> ProjectileClass;

	/** 생성된 투사체에 표시할 메시다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector ProjectileMeshScale = FVector::OneVector;

	/** 아이템 메시의 이 소켓에서 생성한다. 비우면 아이템 Actor 원점을 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch")
	FName LaunchSocketName = NAME_None;

	/** Pawn의 BaseAimRotation을 사용한다. false면 Source Actor 정면을 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch")
	bool bUseSourceAim = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Projectile",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float CollisionRadius = 18.0f;

	/** 선택한 발사 방향을 기준으로 한 생성 거리다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch")
	float SpawnForwardOffset = 75.0f;

	/** 소스 Actor의 위쪽을 기준으로 한 생성 높이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch")
	float SpawnUpOffset = 20.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ForwardSpeed = 1200.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float UpwardSpeed = 350.0f;

	/** true면 Press 동안 충전하고 Release 때 투척한다. ForwardSpeed가 최대 속도다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Launch|Charge")
	bool bEnableCharge = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch|Charge",
		meta = (
			EditCondition = "bEnableCharge",
			EditConditionHides,
			ClampMin = "0.0",
			UIMin = "0.0"))
	float MinimumForwardSpeed = 500.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch|Charge",
		meta = (
			EditCondition = "bEnableCharge",
			EditConditionHides,
			ClampMin = "0.01",
			UIMin = "0.1",
			UIMax = "3.0"))
	float MaximumChargeDuration = 1.5f;

	/** 소유 클라이언트에 첫 Blocking Hit까지의 점선 궤적과 착탄 마커를 표시한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch|Charge|Preview",
		meta = (EditCondition = "bEnableCharge", EditConditionHides))
	bool bShowTrajectoryPreview = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch|Charge|Preview",
		meta = (
			EditCondition = "bEnableCharge && bShowTrajectoryPreview",
			EditConditionHides,
			ClampMin = "0.1",
			UIMin = "0.5",
			UIMax = "5.0"))
	float PreviewMaximumSimulationTime = 3.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch|Charge|Preview",
		meta = (
			EditCondition = "bEnableCharge && bShowTrajectoryPreview",
			EditConditionHides,
			ClampMin = "1.0",
			ClampMax = "60.0",
			UIMin = "10.0",
			UIMax = "30.0"))
	float PreviewSimulationFrequency = 20.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Launch|Charge|Preview",
		meta = (
			EditCondition = "bEnableCharge && bShowTrajectoryPreview",
			EditConditionHides))
	TEnumAsByte<ECollisionChannel> PreviewTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool bShouldBounce = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Movement",
		meta = (EditCondition = "bShouldBounce", ClampMin = "0.0", ClampMax = "1.0"))
	float Bounciness = 0.55f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Movement",
		meta = (EditCondition = "bShouldBounce", ClampMin = "0.0", ClampMax = "1.0"))
	float Friction = 0.2f;

	float CalculateChargeAlpha(float HeldDuration) const;
	float ResolveForwardSpeed(float ChargeAlpha) const;
	bool Validate(FString& OutError) const;
};

/** 생성 뒤 원본 아이템과 독립적으로 유지되는 폭발 결과 설정이다. */
USTRUCT(BlueprintType)
struct PADO_API FPDProjectileExplosionConfigStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Detonation")
	EPDProjectileDetonationMode DetonationMode =
		EPDProjectileDetonationMode::OnFuse;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Detonation",
		meta = (
			EditCondition = "DetonationMode != EPDProjectileDetonationMode::OnImpact",
			EditConditionHides,
			ClampMin = "0.01"))
	float FuseDuration = 2.5f;

	/** 충돌하지 못한 투사체가 영구히 남지 않게 하는 안전 수명이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Detonation",
		meta = (ClampMin = "0.1"))
	float MaximumLifetime = 15.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Explosion",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ExplosionRadius = 300.0f;

	/** 0이면 반경 안의 모든 유효한 ASC 대상을 처리한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Explosion",
		meta = (ClampMin = "0"))
	int32 MaximumTargets = 0;

	/** false면 던진 플레이어는 폭발 결과에서 제외한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion")
	bool bAffectInstigator = false;

	/** true면 Visibility Trace가 가려진 대상에는 Target Fragment를 실행하지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Explosion")
	bool bRequireLineOfSight = false;

	/** 개발용. 폭발 반경과 실제로 처리한 대상을 월드에 그린다. Shipping에서는 그리지 않는다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (DisplayName = "Draw Debug Explosion"))
	bool bDrawDebugExplosion = false;

	/** 디버그 도형이 화면에 남는 시간(초). */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (EditCondition = "bDrawDebugExplosion", ClampMin = "0.0", Units = "s"))
	float DebugDrawDuration = 1.0f;

	bool UsesFuse() const;
	bool ExplodesOnImpact() const;
	bool Validate(FString& OutError) const;
};
