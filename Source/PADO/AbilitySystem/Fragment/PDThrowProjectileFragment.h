#pragma once

#include "CoreMinimal.h"
#include "PADO/AbilitySystem/Fragment/PDActionFragment.h"
#include "PADO/AbilitySystem/Struct/PDProjectileConfigStruct.h"
#include "PDThrowProjectileFragment.generated.h"

/** OnExecute에서 투사체를 서버 권위로 생성하는 일회성 결과 Fragment다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Throw Projectile"))
class PADO_API UPDThrowProjectileFragment : public UPDActionFragment
{
	GENERATED_BODY()

public:
	UPDThrowProjectileFragment();

	virtual bool Validate(FString& OutError) const override;
	virtual bool DeclaresSetByCallerTag(FGameplayTag DataTag) const override;
	virtual void AppendDeclaredSetByCallerTags(
		FGameplayTagContainer& OutTags) const override;
	virtual bool CanExecute(
		const FPDActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FPDActionExecutionContext& Context) const override;

	/** 실제 투척과 로컬 궤적 미리보기가 공유하는 발사 위치·속도 계산이다. */
	bool BuildLaunchSolution(
		const AActor* SourceActor,
		const AActor* LaunchOrigin,
		float ChargeAlpha,
		FTransform& OutSpawnTransform,
		FVector& OutInitialVelocity) const;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Projectile",
		meta = (ShowOnlyInnerProperties))
	FPDProjectileLaunchConfigStruct LaunchConfig;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Projectile",
		meta = (ShowOnlyInnerProperties))
	FPDProjectileExplosionConfigStruct ExplosionConfig;

	/** 폭발 반경의 각 대상에게 지연 실행할 Target 전용 Fragment 목록이다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Projectile|Target Effects")
	TArray<TObjectPtr<UPDActionFragment>> ExplosionTargetFragments;

	/**
	 * 폭발 순간에 한 번 실행할 Source Scope 연출 Fragment 목록이다.
	 * 다른 Action과 같게 Execute Gameplay Cue로 VFX·SFX·카메라 셰이크를 재생한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Projectile|Presentation")
	TArray<TObjectPtr<UPDActionFragment>> ExplosionPresentationFragments;

private:
	AActor* ResolveLaunchOrigin(const FPDActionExecutionContext& Context) const;
	UObject* ResolveEffectSourceObject(
		const FPDActionExecutionContext& Context,
		AActor* LaunchOrigin) const;
};
