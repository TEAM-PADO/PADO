#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "PDGA_Base.generated.h"

class AActor;
class UAbilitySystemComponent;
class UPDAbilityDefinition;
struct FPDActionExecutionContext;
struct FPDGameplayEffectRecipeStruct;

/** Definition의 Action Hook을 실행하는 소스 독립적인 GA 기반 클래스다. */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "PDGA_Base"))
class PADO_API UPDGA_Base : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPDGA_Base();

	bool ValidateDefinitionContract(
		const UPDAbilityDefinition& Definition,
		FString& OutError) const;

	/** ApplyGameplayEffect Fragment가 사용하는 공용 GE 적용 진입점이다. */
	bool ApplyGameplayEffectRecipe(
		const FPDGameplayEffectRecipeStruct& Recipe,
		const FPDActionExecutionContext& Context,
		UAbilitySystemComponent* TargetAbilitySystem,
		bool bTrackUntilAbilityEnds);

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * Definition이 정한 쿨다운 태그를 소유자가 이미 들고 있으면 활성화를 막는다.
	 *
	 * GA 클래스 하나를 모든 아이템이 공유하므로 CDO의 CooldownGameplayEffectClass에
	 * 고정하지 않고 매번 Spec의 SourceObject에서 Definition을 해석한다.
	 */
	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** CommitAbility 시점에 서버가 쿨다운 GE를 소유자에게 적용한다. */
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	void AddSupportedActionHook(FGameplayTag HookTag);
	void AddRequiredActionHook(FGameplayTag HookTag);


	/** ShotResult는 이번 발이 멈춘 곳이다. OnExecuteStart만 넘긴다. */
	bool ExecuteActionHook(
		FGameplayTag HookTag,
		UAbilitySystemComponent* TargetAbilitySystem,
		AActor* TargetActor,
		const FHitResult* HitResult = nullptr,
		const FHitResult* ShotResult = nullptr);

	const UPDAbilityDefinition* GetActiveDefinition() const;
	void SetExecutionChargeAlpha(float ChargeAlpha);
	float GetExecutionChargeAlpha() const;

private:
	struct FPDTrackedActiveEffect
	{
		TWeakObjectPtr<UAbilitySystemComponent> TargetAbilitySystem;
		FActiveGameplayEffectHandle EffectHandle;
	};

	bool ResolveDefinitionForSpec(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const UPDAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr) const;

	FGameplayEffectSpecHandle MakeEffectSpec(
		const FPDGameplayEffectRecipeStruct& Recipe) const;

	UPROPERTY(Transient)
	TObjectPtr<UPDAbilityDefinition> ActiveDefinition;

	TArray<FPDTrackedActiveEffect> TrackedActiveEffects;
	FGameplayTagContainer SupportedActionHooks;
	FGameplayTagContainer RequiredActionHooks;
	float ExecutionChargeAlpha = 1.0f;
};
