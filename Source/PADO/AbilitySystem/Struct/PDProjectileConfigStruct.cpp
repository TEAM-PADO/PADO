#include "PADO/AbilitySystem/Struct/PDProjectileConfigStruct.h"

#include "PADO/AbilitySystem/Projectile/PDActionProjectile.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X) &&
			FMath::IsFinite(Value.Y) &&
			FMath::IsFinite(Value.Z) &&
			!Value.ContainsNaN();
	}
}

float FPDProjectileLaunchConfigStruct::CalculateChargeAlpha(
	float HeldDuration) const
{
	if (!bEnableCharge)
	{
		return 1.0f;
	}

	return FMath::Clamp(
		HeldDuration / FMath::Max(MaximumChargeDuration, UE_SMALL_NUMBER),
		0.0f,
		1.0f);
}

float FPDProjectileLaunchConfigStruct::ResolveForwardSpeed(
	float ChargeAlpha) const
{
	return bEnableCharge
		? FMath::Lerp(
			MinimumForwardSpeed,
			ForwardSpeed,
			FMath::Clamp(ChargeAlpha, 0.0f, 1.0f))
		: ForwardSpeed;
}

bool FPDProjectileLaunchConfigStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!ProjectileClass ||
		ProjectileClass->HasAnyClassFlags(CLASS_Abstract))
	{
		OutError = TEXT("ProjectileClass에는 생성 가능한 투사체 Actor가 필요합니다.");
		return false;
	}

	if (!ProjectileMesh)
	{
		OutError = TEXT("ProjectileMesh가 비어 있습니다.");
		return false;
	}

	if (!IsFiniteVector(ProjectileMeshScale) ||
		ProjectileMeshScale.X <= 0.0f ||
		ProjectileMeshScale.Y <= 0.0f ||
		ProjectileMeshScale.Z <= 0.0f)
	{
		OutError = TEXT("ProjectileMeshScale의 각 축은 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(CollisionRadius) || CollisionRadius <= 0.0f)
	{
		OutError = TEXT("CollisionRadius는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(SpawnForwardOffset) ||
		!FMath::IsFinite(SpawnUpOffset) ||
		!FMath::IsFinite(ForwardSpeed) || ForwardSpeed < 0.0f ||
		!FMath::IsFinite(UpwardSpeed) || UpwardSpeed < 0.0f ||
		!FMath::IsFinite(GravityScale) || GravityScale < 0.0f)
	{
		OutError = TEXT("생성 오프셋과 이동 수치는 유효한 범위의 유한한 수여야 합니다.");
		return false;
	}

	if (FMath::IsNearlyZero(ForwardSpeed) && FMath::IsNearlyZero(UpwardSpeed))
	{
		OutError = TEXT("ForwardSpeed와 UpwardSpeed가 모두 0입니다.");
		return false;
	}

	if (bEnableCharge &&
		(!FMath::IsFinite(MinimumForwardSpeed) ||
		 MinimumForwardSpeed < 0.0f ||
		 MinimumForwardSpeed > ForwardSpeed ||
		 !FMath::IsFinite(MaximumChargeDuration) ||
		 MaximumChargeDuration <= 0.0f))
	{
		OutError = TEXT("충전 투척은 0 이상 최대 ForwardSpeed 이하의 MinimumForwardSpeed와 0보다 큰 MaximumChargeDuration이 필요합니다.");
		return false;
	}

	if (bEnableCharge && bShowTrajectoryPreview &&
		(!FMath::IsFinite(PreviewMaximumSimulationTime) ||
		 PreviewMaximumSimulationTime <= 0.0f ||
		 !FMath::IsFinite(PreviewSimulationFrequency) ||
		 PreviewSimulationFrequency < 1.0f ||
		 PreviewSimulationFrequency > 60.0f))
	{
		OutError = TEXT("궤적 미리보기의 Simulation Time은 0보다 크고 Frequency는 1~60이어야 합니다.");
		return false;
	}

	if (bShouldBounce &&
		(!FMath::IsFinite(Bounciness) || Bounciness < 0.0f ||
		 Bounciness > 1.0f || !FMath::IsFinite(Friction) ||
		 Friction < 0.0f || Friction > 1.0f))
	{
		OutError = TEXT("Bounciness와 Friction은 0~1 사이의 유한한 수여야 합니다.");
		return false;
	}

	return true;
}

bool FPDProjectileExplosionConfigStruct::UsesFuse() const
{
	return DetonationMode == EPDProjectileDetonationMode::OnFuse ||
		DetonationMode == EPDProjectileDetonationMode::OnImpactOrFuse;
}

bool FPDProjectileExplosionConfigStruct::ExplodesOnImpact() const
{
	return DetonationMode == EPDProjectileDetonationMode::OnImpact ||
		DetonationMode == EPDProjectileDetonationMode::OnImpactOrFuse;
}

bool FPDProjectileExplosionConfigStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!FMath::IsFinite(MaximumLifetime) || MaximumLifetime <= 0.0f)
	{
		OutError = TEXT("MaximumLifetime은 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (UsesFuse() &&
		(!FMath::IsFinite(FuseDuration) || FuseDuration <= 0.0f ||
		 FuseDuration >= MaximumLifetime))
	{
		OutError = TEXT("FuseDuration은 0보다 크고 MaximumLifetime보다 작아야 합니다.");
		return false;
	}

	if (!FMath::IsFinite(ExplosionRadius) || ExplosionRadius <= 0.0f)
	{
		OutError = TEXT("ExplosionRadius는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (MaximumTargets < 0)
	{
		OutError = TEXT("MaximumTargets는 0 이상이어야 합니다.");
		return false;
	}

	return true;
}
