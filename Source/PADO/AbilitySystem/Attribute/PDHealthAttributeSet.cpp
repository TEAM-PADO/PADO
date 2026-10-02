#include "PADO/AbilitySystem/Attribute/PDHealthAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"

namespace PDHealthAttributeDefaults
{
	constexpr float MaxHealth = 100.0f;

	/** 최대 체력이 0이 되면 체력이 늘 0이라 살아 있는 상태가 성립하지 않는다. */
	constexpr float MinMaxHealth = 1.0f;
}

UPDHealthAttributeSet::UPDHealthAttributeSet()
	: Health(PDHealthAttributeDefaults::MaxHealth)
	, MaxHealth(PDHealthAttributeDefaults::MaxHealth)
	, Damage(0.0f)
{
}

void UPDHealthAttributeSet::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 팀원 체력 표시처럼 다른 플레이어도 보므로 모두에게 보낸다.
	DOREPLIFETIME_CONDITION_NOTIFY(UPDHealthAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPDHealthAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

bool UPDHealthAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// 죽은 몸은 더 다치지 않는다. 시체를 쏘거나 폭발에 휘말려도 사망 처리가 다시 일어나지 않는다.
	return Data.EvaluatedData.Attribute != GetDamageAttribute() ||
		!Data.Target.HasMatchingGameplayTag(TAG_PD_State_Dead);
}

void UPDHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute != GetDamageAttribute())
	{
		return;
	}

	const float DamageDone = GetDamage();
	SetDamage(0.0f);
	if (DamageDone <= 0.0f)
	{
		return;
	}

	const float OldHealth = GetHealth();
	const float NewHealth = FMath::Clamp(OldHealth - DamageDone, 0.0f, GetMaxHealth());
	SetHealth(NewHealth);

	// Instigator는 논리적 주체(플레이어면 PlayerState), EffectCauser는 아이템·투사체다.
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetEffectContext();
	OnDamaged.Broadcast(
		Context.GetOriginalInstigator(),
		Context.GetEffectCauser(),
		Data.EffectSpec,
		DamageDone,
		OldHealth,
		NewHealth);
}

void UPDHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UPDHealthAttributeSet::PreAttributeBaseChange(
	const FGameplayAttribute& Attribute,
	float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UPDHealthAttributeSet::PostAttributeChange(
	const FGameplayAttribute& Attribute,
	float OldValue,
	float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 최대 체력이 줄면 넘치는 체력을 깎는다.
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		if (UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent())
		{
			AbilitySystem->ApplyModToAttribute(
				GetHealthAttribute(),
				EGameplayModOp::Override,
				NewValue);
		}
	}
}

void UPDHealthAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPDHealthAttributeSet, Health, OldHealth);
}

void UPDHealthAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPDHealthAttributeSet, MaxHealth, OldMaxHealth);
}

void UPDHealthAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, PDHealthAttributeDefaults::MinMaxHealth);
	}
}
