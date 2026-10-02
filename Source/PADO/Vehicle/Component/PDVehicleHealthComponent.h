#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PDVehicleHealthComponent.generated.h"

class UAbilitySystemComponent;
class UPDHealthAttributeSet;
struct FGameplayEffectSpec;

/** 탈것의 파괴 상태와 그 상태를 만든 주체다. 한 구조체로 복제해 도착 순서가 어긋나지 않게 한다. */
USTRUCT()
struct FPDVehicleDestructionStruct
{
	GENERATED_BODY()

	UPROPERTY()
	bool bDestroyed = false;

	/** 파괴한 논리적 주체다(플레이어면 PlayerState). */
	UPROPERTY()
	TObjectPtr<AActor> Instigator = nullptr;

	/** 파괴한 물리적 원인이다(아이템·투사체). */
	UPROPERTY()
	TObjectPtr<AActor> EffectCauser = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDVehicleDestroyedSignature);

/**
 * 탈것의 체력과 파괴 상태다. 체력은 탈것 자신의 ASC에 있는 UPDHealthAttributeSet이다.
 *
 * 서버가 시작할 때 최대 체력을 채우고, 체력이 0이 되면 파괴한다. 파괴는 되돌리지 않는다.
 * - 이후 피해를 받지 않는다(탈것 ASC에 State.Dead).
 * - 타고 있던 사람은 모두 사망하고 내린다. 운전자가 사라지므로 차는 중립 입력으로 굴러간다.
 * - 새로 탈 수 없다(APDWheeledVehicle::CanInteract).
 * - 이 머신에 파괴가 적용될 때 OnVehicleDestroyed로 알린다. 잔해·불 같은 연출은 여기에 붙인다.
 */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDVehicleHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDVehicleHealthComponent();

	UFUNCTION(BlueprintPure, Category = "PD|Vehicle|Health")
	bool IsDestroyed() const { return Destruction.bDestroyed; }

	/** 이 머신에 파괴가 적용될 때 한 번 발생한다. 늦게 접속한 머신에서도 발생한다. */
	UPROPERTY(BlueprintAssignable, Category = "PD|Vehicle|Health")
	FPDVehicleDestroyedSignature OnVehicleDestroyed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Destruction();

	/** 시작 체력이자 최대 체력이다. 돌격소총(20) 50발, 저격(75) 14발이 기본값이다. */
	UPROPERTY(EditDefaultsOnly, Category = "PD|Vehicle|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 1000.0f;

private:
	void HandleDamaged(
		AActor* Instigator,
		AActor* EffectCauser,
		const FGameplayEffectSpec& EffectSpec,
		float Damage,
		float OldHealth,
		float NewHealth);

	/** 이 머신에 파괴를 적용한다. 같은 상태가 다시 와도 한 번만 적용한다. */
	void ApplyDestruction();

	/** 서버에서 타고 있던 사람을 모두 사망시킨다. 죽은 몸은 사망 처리가 내리게 한다. */
	void KillOccupants() const;

	UAbilitySystemComponent* FindAbilitySystem() const;

	UPROPERTY(ReplicatedUsing = OnRep_Destruction)
	FPDVehicleDestructionStruct Destruction;

	/** 이 머신에 파괴를 적용했는지다. */
	bool bDestructionApplied = false;

	TWeakObjectPtr<const UPDHealthAttributeSet> HealthSet;
	FDelegateHandle DamagedHandle;
};
