#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "PDGA_Base.generated.h"

class AActor;
class UAbilitySystemComponent;
class UPDAbilityDefinition;
struct FPDActionExecutionContext;
struct FPDActionHookStruct;
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

	/**
	 * 입력을 조종하는 머신 안에서만 쓰는가. 그런 Ability는 활성 중인 Press와
	 * Release를 서버로 보내지 않고 자기 인스턴스에만 전달한다. Press도 한 번의
	 * 실행이 아니므로 입력 계층이 Press를 발사로 세지 않는다.
	 */
	virtual bool UsesLocalTriggerInput() const;

	/**
	 * Hook 하나의 Fragment를 문맥의 실행 범위대로 골라 실행한다.
	 *
	 * GA 인스턴스가 없는 머신(Fire Action의 관찰자)도 같은 규칙으로 실행해야 하므로
	 * 정적 함수다. 필수 Fragment가 실패하면 false다.
	 */
	static bool ExecuteHookFragments(
		FGameplayTag HookTag,
		const FPDActionHookStruct& Hook,
		const FPDActionExecutionContext& Context);

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * Definition이 정한 쿨다운이 아직 끝나지 않았으면 활성화를 막는다.
	 *
	 * 조종하는 머신만 판정하고, 자기가 시작한 시각을 기준으로 한다. 다른 머신이
	 * 조종하는 요청은 서버가 다시 막지 않는다. 서버 쿨다운은 요청이 도착한 뒤에
	 * 시작해 늦게 끝나므로, 막으면 지연만큼 정상 요청을 거부하게 된다.
	 *
	 * GA 클래스 하나를 모든 아이템이 공유하므로 CDO의 CooldownGameplayEffectClass에
	 * 고정하지 않고 매번 Spec의 SourceObject에서 Definition을 해석한다.
	 */
	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * CommitAbility 시점에 쿨다운 GE를 소유자에게 적용하고, 조종하는 머신이면
	 * 판정에 쓸 로컬 쿨다운을 시작한다. GE는 다른 머신에 보여 줄 상태다.
	 */
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

	/** 상위 클래스가 등록한 Hook 계약을 버리고 파생 클래스가 새로 정한다. */
	void ResetActionHookContract();


	/** ShotResult는 이번 발이 멈춘 곳이다. OnExecuteStart만 넘긴다. */
	bool ExecuteActionHook(
		FGameplayTag HookTag,
		UAbilitySystemComponent* TargetAbilitySystem,
		AActor* TargetActor,
		const FHitResult* HitResult = nullptr,
		const FHitResult* ShotResult = nullptr);

	/** 이 인스턴스의 Hook 실행 문맥이다. 실행 범위는 이 머신의 권한으로 정한다. */
	FPDActionExecutionContext MakeHookContext(
		UAbilitySystemComponent* TargetAbilitySystem,
		AActor* TargetActor,
		const FHitResult* HitResult,
		const FHitResult* ShotResult) const;

	/**
	 * Source 기준 필수 Fragment의 전제 조건을 복제된 상태로 본다. 탄약처럼 실행
	 * 비용이 남아 있는지다. 서버와 조종하는 머신이 같은 판정을 한다.
	 */
	bool PassesPredictedStateGate(
		const UPDAbilityDefinition& Definition,
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo) const;

	/**
	 * 활성화가 곧 실행인가. 그렇다면 실행 비용을 활성화 전에 본다. Fire Action처럼
	 * 활성화 안에서 여러 번 실행하는 Ability는 실행마다 따로 본다.
	 */
	virtual bool GatesActivationOnPredictedState() const;

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
