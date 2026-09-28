#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "PADO/AbilitySystem/Struct/PDFireShotStruct.h"
#include "PDAbilitySystemComponent.generated.h"

struct FGameplayEventData;
class UAnimMontage;
class UPDAbilityDefinition;

/** 소스 독립적인 Ability Binding을 정확한 SpecHandle 단위로 관리한다. */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Grant 시점 전용이다. Definition 전체를 재귀 검증하므로 비용이 크다. */
	static bool ResolveDefinitionFromSource(
		const UObject* SourceObject,
		const UPDAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr);

	/**
	 * 검증 없이 소스가 제공하는 Definition만 가져온다.
	 * Definition은 Grant 시점에 이미 검증됐고 Grant 중에는 교체할 수 없으므로,
	 * 매 활성화마다 재검증하지 않는다.
	 */
	static bool GetDefinitionFromSource(
		const UObject* SourceObject,
		const UPDAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr);

	UFUNCTION(BlueprintCallable, Category = "PD|Ability")
	FGameplayAbilitySpecHandle GrantAbilityFromSource(
		UObject* SourceObject,
		int32 InputId = -1);

	FGameplayAbilitySpecHandle GrantAbilityDefinition(
		const UPDAbilityDefinition* Definition,
		UObject* SourceObject,
		int32 InputId = INDEX_NONE);

	UFUNCTION(BlueprintCallable, Category = "PD|Ability")
	bool TryActivateGrantedAbility(FGameplayAbilitySpecHandle AbilityHandle);

	/**
	 * 정확한 SpecHandle의 입력을 누르고, 비활성 상태면 같은 요청에서 활성화한다.
	 * 소유 클라이언트는 서버를 기다리지 않고 직접 활성화한다. 반환값은 이 머신에서
	 * 실제로 활성화됐는지다. 서버가 나중에 거부할 수는 있다.
	 *
	 * 입력을 로컬 방아쇠로만 쓰는 Ability(Fire Action)는 활성 중인 인스턴스에만
	 * 전달하고 서버로 보내지 않는다. 반환값은 전달했는지다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Ability|Input")
	bool PressAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	/**
	 * Press 때 사용한 정확한 SpecHandle의 입력을 놓는다. 소유 클라이언트는 서버와
	 * 자기 인스턴스 모두에 알린다. 로컬 방아쇠는 자기 인스턴스에만 알린다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Ability|Input")
	bool ReleaseAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	/**
	 * 조종하는 클라이언트가 쏜 발 묶음을 서버로 보낸다. 발 기록은 탄약과 피해를
	 * 바꾸므로 신뢰성 RPC다. 같은 Actor 채널의 다른 요청과 호출 순서대로 도착한다.
	 */
	UFUNCTION(Server, Reliable)
	void ServerSubmitFireShots(const FPDFireShotBatchStruct& Batch);

	/**
	 * 서버가 처리한 발을 관찰자에게 전달한다. 연출용이라 잃어도 결과는 같다.
	 * GAS의 일회성 Cue 멀티캐스트와 같은 이유로 비신뢰성이다.
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireShots(const FPDFireShotBatchStruct& Batch);

	bool TryActivateGrantedAbilityWithEvent(
		FGameplayAbilitySpecHandle AbilityHandle,
		FGameplayTag EventTag,
		const FGameplayEventData& EventData);

	/**
	 * 조종하는 머신이 자기 시계로 Action 쿨다운을 시작한다.
	 *
	 * 서버의 쿨다운 GE는 요청이 도착한 시점에 시작하므로, 복제되면 소유
	 * 클라이언트에서는 지연의 절반만큼 늦게 끝난다. 그 값으로 판정하면 지연이
	 * 클수록 쿨다운이 길어진다. 그래서 조종하는 머신은 이 기록으로 판정하고,
	 * 쿨다운 GE는 다른 머신에 보여 줄 상태로만 남는다.
	 */
	void BeginLocalActionCooldown(FGameplayTag CooldownTag, float Duration);

	/** 조종하는 머신의 쿨다운 판정이다. 자기가 시작한 시각 기준이다. */
	bool IsLocalActionCooldownActive(FGameplayTag CooldownTag) const;

	/**
	 * 쿨다운 UI가 볼 남은 시간이다. 조종하는 머신은 판정과 같은 로컬 기록을,
	 * 다른 머신은 서버가 복제한 쿨다운 GE를 본다.
	 */
	UFUNCTION(BlueprintPure, Category = "PD|Ability|Cooldown")
	float GetActionCooldownRemaining(FGameplayTag CooldownTag) const;

	UFUNCTION(BlueprintCallable, Category = "PD|Ability")
	bool RevokeAbilityByHandle(
		FGameplayAbilitySpecHandle AbilityHandle,
		bool bCancelActiveAbility = true);

	FGameplayAbilitySpecHandle FindGrantedAbilityBySource(
		const UObject* SourceObject) const;

	/**
	 * 조종 중인 원격 클라이언트의 선재생 Action Montage를 승인하거나 재생한다.
	 * ASC의 몽타주 복제는 OnRep_ReplicatedAnimMontage에서 자기 자신을 건너뛰므로,
	 * ServerOnly Ability로 실행한 사용 동작을 정작 사용한 본인이 볼 수 없다.
	 * 판정은 서버가 단독으로 확정하며 요청 번호는 표현 중복과 늦은 응답만 막는다.
	 */
	/**
	 * 서버가 확정한 짧은 재생률 변경을 원격 소유자에게도 적용한다.
	 *
	 * 몽타주 자체는 LocalPredicted 어빌리티가 클라이언트에서 직접 재생한다.
	 * 역경직은 명중이 확정된 뒤에야 알 수 있으므로 서버가 따로 알려야 한다.
	 */
	void ApplyActionMontageHitLagForRemoteOwner(
		UAnimMontage* Montage,
		float EffectivePlayRate,
		float Duration,
		uint32 InHitLagGeneration);

protected:
	UFUNCTION(Server, Reliable)
	void ServerReleaseAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	UFUNCTION(Client, Reliable)
	void ClientApplyActionMontageHitLag(
		UAnimMontage* Montage,
		float EffectivePlayRate,
		float Duration,
		uint32 InHitLagGeneration);

private:
	/** 입력을 로컬 방아쇠로만 쓰는 Ability인가. */
	static bool UsesLocalTriggerInput(const FGameplayAbilitySpec& Spec);

	bool ProcessAbilityInputPressed(FGameplayAbilitySpecHandle AbilityHandle);
	bool ProcessAbilityInputReleased(FGameplayAbilitySpecHandle AbilityHandle);
	bool IsRemoteOwnerMontageTarget() const;
	void RestoreHitLagPlayRate();
	void ClearHitLag();

	/** 역경직으로 재생률을 낮춘 몽타주다. 복원 대상이다. */
	TWeakObjectPtr<UAnimMontage> HitLagMontage;

	/** 역경직 전 재생률이다. 복원할 때 되돌린다. */
	float HitLagRestorePlayRate = 1.0f;

	FTimerHandle HitLagTimerHandle;

	/** 서버가 붙인 순번이다. 늦게 도착한 오래된 역경직을 무시한다. */
	uint32 HitLagGeneration = 0;

	/** 조종하는 머신이 기록한 쿨다운 태그별 종료 시각(월드 시간)이다. 복제하지 않는다. */
	TMap<FGameplayTag, double> LocalActionCooldownEndTimes;
};
