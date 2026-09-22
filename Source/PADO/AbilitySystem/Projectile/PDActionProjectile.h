#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Fragment/PDActionExecutionContext.h"
#include "PADO/AbilitySystem/Struct/PDProjectileConfigStruct.h"
#include "PDActionProjectile.generated.h"

class APawn;
class UAbilitySystemComponent;
class UPDActionFragment;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 아이템 수명과 분리되어 서버에서 비행·충돌·폭발 결과를 처리하는 공용 투사체다.
 * Throw Projectile Fragment가 설정을 주입하며 BP 파생 클래스로 표현을 확장할 수 있다.
 */
UCLASS(Blueprintable)
class PADO_API APDActionProjectile : public AActor
{
	GENERATED_BODY()

public:
	APDActionProjectile();

	/** FinishSpawning 이후 서버에서 한 번 호출한다. */
	bool InitializeProjectile(
		const FPDProjectileLaunchConfigStruct& LaunchConfig,
		const FPDProjectileExplosionConfigStruct& ExplosionConfig,
		const TArray<TObjectPtr<UPDActionFragment>>& ExplosionTargetFragments,
		const TArray<TObjectPtr<UPDActionFragment>>& ExplosionPresentationFragments,
		UAbilitySystemComponent* SourceAbilitySystem,
		UObject* EffectSourceObject,
		AActor* SourceActor,
		APawn* SourcePawn,
		const FVector& InitialVelocity);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Projectile")
	bool Detonate();

	UFUNCTION(BlueprintPure, Category = "PD|Projectile")
	USphereComponent* GetCollisionComponent() const;

	UFUNCTION(BlueprintPure, Category = "PD|Projectile")
	UProjectileMovementComponent* GetProjectileMovement() const;

	UFUNCTION(BlueprintPure, Category = "PD|Projectile")
	AActor* GetIgnoredSourceActor() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_Owner() override;
	virtual void OnRep_Instigator() override;
	virtual void PostNetReceiveLocationAndRotation() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void HandleBlockingHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void OnRep_Presentation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

private:
	void HandleFuseExpired();
	void ApplyPresentation();
	void GatherExplosionTargets(TArray<AActor*>& OutTargets) const;
	bool HasLineOfSightTo(const AActor* TargetActor) const;
	void ApplyExplosionToTarget(AActor* TargetActor) const;
	/** 폭발 위치에서 한 번 실행하는 연출 Fragment다. Cue 복제는 ASC가 맡는다. */
	void PlayExplosionPresentation() const;
	/** 폭발 반경, 실제 처리한 대상, 반경 안에서 제외된 Pawn을 구분해 그린다. */
	void DrawDebugExplosion(const TArray<AActor*>& Targets) const;
	/** 원본 Fragment를 투사체 소유 Runtime 복제본으로 만들어 지연 실행을 준비한다. */
	bool InitializeExplosionFragments(
		const TArray<TObjectPtr<UPDActionFragment>>& SourceFragments,
		EPDActionScope RequiredScope,
		const TCHAR* ListName,
		UAbilitySystemComponent* SourceAbilitySystem,
		UObject* EffectSourceObject,
		AActor* SourceActor,
		TArray<TObjectPtr<UPDActionFragment>>& OutRuntimeFragments,
		FString& OutError);
	bool ExecuteExplosionFragments(
		const TArray<TObjectPtr<UPDActionFragment>>& RuntimeFragments,
		const FPDActionExecutionContext& Context) const;
	void RefreshSourceMovementIgnore(AActor* SourceActor, APawn* SourcePawn);
	void ClearSourceMovementIgnore();

	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	TObjectPtr<UStaticMesh> ReplicatedProjectileMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	FVector ReplicatedProjectileMeshScale = FVector::OneVector;

	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	float ReplicatedCollisionRadius = 18.0f;

	UPROPERTY(Transient)
	FPDProjectileExplosionConfigStruct ActiveExplosionConfig;

	UPROPERTY(Transient)
	TObjectPtr<UObject> ActiveEffectSourceObject;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ActiveSourceAbilitySystem;

	/** 원본 DA와 Ability 수명에서 분리한 투사체 소유 Runtime 복제본이다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDActionFragment>> ActiveExplosionTargetFragments;

	/** 폭발 위치에서 한 번만 실행하는 Source Scope 연출 복제본이다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPDActionFragment>> ActiveExplosionPresentationFragments;

	TWeakObjectPtr<AActor> IgnoredSourceActor;
	TWeakObjectPtr<APawn> IgnoredSourcePawn;
	FTimerHandle FuseTimerHandle;
	bool bInitialized = false;
	bool bHasDetonated = false;
};
