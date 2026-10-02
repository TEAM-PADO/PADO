#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "PADO/AbilitySystem/Attribute/PDAttributeAccessors.h"
#include "PDHealthAttributeSet.generated.h"

struct FGameplayEffectSpec;

/**
 * 피해가 체력에 반영됐다. 서버에서만 발생한다.
 * 피해를 준 논리적 주체(Instigator), 물리적 원인(EffectCauser), 피해량, 이전·이후 체력이다.
 */
DECLARE_MULTICAST_DELEGATE_SixParams(
	FPDHealthDamagedSignature,
	AActor* /*Instigator*/,
	AActor* /*EffectCauser*/,
	const FGameplayEffectSpec& /*EffectSpec*/,
	float /*Damage*/,
	float /*OldHealth*/,
	float /*NewHealth*/);

/**
 * 체력이다. 플레이어는 PlayerState의 ASC가, 플레이어가 아닌 캐릭터는 자기 ASC가 가진다.
 *
 * 피해는 메타 속성 Damage로 들어오고(UPDGE_Damage), 서버가 체력으로 옮긴다. 체력은
 * 0과 최대 체력 사이로 자른다. 체력이 0이 된 뒤의 처리(빈사, 사망)는 몸의
 * UPDHealthComponent가 정한다. 죽은 대상(State.Dead)은 피해를 받지 않는다.
 */
UCLASS()
class PADO_API UPDHealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPDHealthAttributeSet();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(
		const FGameplayAttribute& Attribute,
		float& NewValue) const override;
	virtual void PostAttributeChange(
		const FGameplayAttribute& Attribute,
		float OldValue,
		float NewValue) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "PD|Health")
	FGameplayAttributeData Health;
	PD_ATTRIBUTE_ACCESSORS(UPDHealthAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "PD|Health")
	FGameplayAttributeData MaxHealth;
	PD_ATTRIBUTE_ACCESSORS(UPDHealthAttributeSet, MaxHealth)

	/** 이번 Effect가 주는 피해량이다. 서버에서 체력으로 옮긴 뒤 0으로 비운다. 복제하지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category = "PD|Health")
	FGameplayAttributeData Damage;
	PD_ATTRIBUTE_ACCESSORS(UPDHealthAttributeSet, Damage)

	/** Attribute Set은 ASC에서 const로 꺼내므로 구독할 수 있게 mutable로 둔다. */
	mutable FPDHealthDamagedSignature OnDamaged;

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
