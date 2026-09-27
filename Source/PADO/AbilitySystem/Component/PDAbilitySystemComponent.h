#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpecHandle.h"
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

	/** 정확한 SpecHandle의 입력을 누르고, 비활성 상태면 같은 요청에서 활성화한다. */
	UFUNCTION(BlueprintCallable, Category = "PD|Ability|Input")
	bool PressAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	/** Press 때 사용한 정확한 SpecHandle의 입력을 놓는다. */
	UFUNCTION(BlueprintCallable, Category = "PD|Ability|Input")
	bool ReleaseAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	bool TryActivateGrantedAbilityWithEvent(
		FGameplayAbilitySpecHandle AbilityHandle,
		FGameplayTag EventTag,
		const FGameplayEventData& EventData);

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
	void ServerPressAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	UFUNCTION(Server, Reliable)
	void ServerReleaseAbilityInputByHandle(FGameplayAbilitySpecHandle AbilityHandle);

	UFUNCTION(Client, Reliable)
	void ClientApplyActionMontageHitLag(
		UAnimMontage* Montage,
		float EffectivePlayRate,
		float Duration,
		uint32 InHitLagGeneration);

private:
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
};
