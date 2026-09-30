#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/AbilitySystem/Targeting/PDActionTargeting.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDAbilitySource, Log, All);

UPDAbilitySourceComponent::UPDAbilitySourceComponent()
{
	SetIsReplicatedByDefault(true);
}

bool UPDAbilitySourceComponent::ResolveAbilityDefinition(
	const UPDAbilityDefinition*& OutDefinition) const
{
	// 런타임 주입값이 있으면 우선하고, 없으면 컴포넌트에 저작된 값을 쓴다.
	OutDefinition = AbilityDefinition
		? AbilityDefinition.Get()
		: ResolveAuthoredActionDefinition();
	return IsValid(OutDefinition);
}

UPDAbilityDefinition* UPDAbilitySourceComponent::GetActionDefinition() const
{
	return ResolveAuthoredActionDefinition();
}

UPDAbilityDefinition*
UPDAbilitySourceComponent::ResolveAuthoredActionDefinition() const
{
	// 명시적인 배치 인스턴스 Override만 클래스 기본값보다 우선한다.
	if (IsValid(InstanceActionDefinition))
	{
		return InstanceActionDefinition;
	}

	// Instanced 중첩 UObject는 레벨 Actor에 복제되어 BP 기본값 변경 뒤에도
	// 예전 복제본이 남을 수 있다. 런타임에는 가장 최신 Owner CDO의
	// 같은 이름 컴포넌트가 가진 Definition을 기준값으로 사용한다.
	AActor* Owner = GetOwner();
	const AActor* OwnerClassDefault = Owner
		? Owner->GetClass()->GetDefaultObject<AActor>()
		: nullptr;
	if (OwnerClassDefault && OwnerClassDefault != Owner)
	{
		TInlineComponentArray<UPDAbilitySourceComponent*> DefaultSources;
		OwnerClassDefault->GetComponents(DefaultSources);
		for (const UPDAbilitySourceComponent* DefaultSource : DefaultSources)
		{
			if (DefaultSource && DefaultSource->GetFName() == GetFName())
			{
				return DefaultSource->ActionDefinition;
			}
		}
	}

	// CDO·컴포넌트 템플릿·독립 테스트 오브젝트는 자기 저작값을 쓴다.
	return ActionDefinition;
}

bool UPDAbilitySourceComponent::GrantToOwner()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return false;
	}

	UPDAbilitySystemComponent* OwnerAbilitySystem =
		Cast<UPDAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner));
	if (!OwnerAbilitySystem)
	{
		UE_LOG(
			LogPDAbilitySource,
			Warning,
			TEXT("Owner '%s'에 PD ASC가 없어 Ability를 부여하지 못했습니다."),
			*GetNameSafe(Owner));
		return false;
	}

	return GrantToAbilitySystem(OwnerAbilitySystem);
}

bool UPDAbilitySourceComponent::ConfigureAbilityDefinition(
	UPDAbilityDefinition* NewDefinition)
{
	const AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority() &&
		BindingState.AbilityHandle.IsValid() && AbilityDefinition != NewDefinition)
	{
		return false;
	}

	AbilityDefinition = NewDefinition;
	return true;
}

bool UPDAbilitySourceComponent::GrantToAbilitySystem(
	UPDAbilitySystemComponent* AbilitySystem)
{
	if (!AbilitySystem || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (BindingState.AbilityHandle.IsValid())
	{
		UPDAbilitySystemComponent* ExistingAbilitySystem =
			GetGrantedAbilitySystem();
		if (ExistingAbilitySystem == AbilitySystem &&
			AbilitySystem->FindAbilitySpecFromHandle(
				BindingState.AbilityHandle))
		{
			return true;
		}

		if (ExistingAbilitySystem &&
			ExistingAbilitySystem != AbilitySystem &&
			!Revoke(true))
		{
			return false;
		}
		else if (!ExistingAbilitySystem ||
			ExistingAbilitySystem == AbilitySystem)
		{
			BindingState.Reset();
			AuthorityGrantedAbilitySystem = nullptr;
		}
	}

	const FGameplayAbilitySpecHandle NewHandle =
		AbilitySystem->GrantAbilityFromSource(this);
	if (!NewHandle.IsValid())
	{
		return false;
	}

	BindingState.AbilityHandle = NewHandle;
	BindingState.AbilitySystemOwner = AbilitySystem->GetOwnerActor();
	AuthorityGrantedAbilitySystem = AbilitySystem;
	return true;
}

bool UPDAbilitySourceComponent::TryActivate()
{
	UPDAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	if (!AbilitySystem || !CanActivateWithoutTarget())
	{
		return false;
	}

	return AbilitySystem->TryActivateGrantedAbility(BindingState.AbilityHandle);
}

bool UPDAbilitySourceComponent::PressInput(
	FGameplayAbilitySpecHandle& OutPressedHandle)
{
	OutPressedHandle = FGameplayAbilitySpecHandle();
	UPDAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	if (!AbilitySystem || !CanActivateWithoutTarget())
	{
		return false;
	}

	const FGameplayAbilitySpecHandle PressedHandle = BindingState.AbilityHandle;
	if (!AbilitySystem->PressAbilityInputByHandle(PressedHandle))
	{
		return false;
	}

	OutPressedHandle = PressedHandle;
	return true;
}

bool UPDAbilitySourceComponent::ReleaseInput(
	FGameplayAbilitySpecHandle PressedHandle)
{
	UPDAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	return AbilitySystem &&
		AbilitySystem->ReleaseAbilityInputByHandle(PressedHandle);
}

bool UPDAbilitySourceComponent::CanActivateWithoutTarget() const
{
	const UPDAbilityDefinition* Definition = nullptr;
	if (!ResolveAbilityDefinition(Definition))
	{
		return false;
	}

	const UPDActionTargeting* Targeting = Definition->ActionTargeting;
	if (Targeting && Targeting->RequiresActivationTarget())
	{
		UE_LOG(
			LogPDAbilitySource,
			Warning,
			TEXT("'%s'는 활성화 Target이 필요해 TryActivate()로 발동할 수 없습니다. ")
			TEXT("TryActivateWithTarget()을 사용하세요. Source='%s'"),
			*Targeting->GetClass()->GetName(),
			*GetNameSafe(GetOwner()));
		return false;
	}

	return true;
}

bool UPDAbilitySourceComponent::TryActivateWithTarget(AActor* TargetActor)
{
	return TryActivateWithTargetInternal(TargetActor, nullptr);
}

bool UPDAbilitySourceComponent::TryActivateWithHitResult(
	AActor* TargetActor,
	const FHitResult& HitResult)
{
	return TryActivateWithTargetInternal(TargetActor, &HitResult);
}

bool UPDAbilitySourceComponent::TryActivateWithTargetInternal(
	AActor* TargetActor,
	const FHitResult* HitResult)
{
	if (!TargetActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	UPDAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	if (!AbilitySystem)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.EventTag = TAG_PD_GameplayEvent_AbilitySource_Trigger;
	EventData.Instigator = AbilitySystem->GetAvatarActor();
	EventData.Target = TargetActor;
	EventData.OptionalObject = this;
	if (HitResult)
	{
		// Instigator는 논리적 주체(ASC 소유자), EffectCauser는 이 Source를 가진 액터다.
		EventData.ContextHandle = AbilitySystem->MakeEffectContext();
		EventData.ContextHandle.AddInstigator(
			AbilitySystem->GetOwnerActor(),
			GetOwner());
		EventData.ContextHandle.AddHitResult(*HitResult, true);
	}

	return AbilitySystem->TryActivateGrantedAbilityWithEvent(
		BindingState.AbilityHandle,
		EventData.EventTag,
		EventData);
}

bool UPDAbilitySourceComponent::Revoke(bool bCancelActiveAbility)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!BindingState.AbilityHandle.IsValid())
	{
		BindingState.Reset();
		AuthorityGrantedAbilitySystem = nullptr;
		return true;
	}

	UPDAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	if (!AbilitySystem)
	{
		if (!IsValid(BindingState.AbilitySystemOwner))
		{
			BindingState.Reset();
			AuthorityGrantedAbilitySystem = nullptr;
			return true;
		}
		return false;
	}

	if (!AbilitySystem->FindAbilitySpecFromHandle(BindingState.AbilityHandle))
	{
		BindingState.Reset();
		AuthorityGrantedAbilitySystem = nullptr;
		return true;
	}

	if (!AbilitySystem->RevokeAbilityByHandle(
		BindingState.AbilityHandle,
		bCancelActiveAbility))
	{
		return false;
	}

	BindingState.Reset();
	AuthorityGrantedAbilitySystem = nullptr;
	return true;
}

FGameplayAbilitySpecHandle UPDAbilitySourceComponent::GetGrantedAbilityHandle() const
{
	return BindingState.AbilityHandle;
}

bool UPDAbilitySourceComponent::HasAbilityDefinition() const
{
	const UPDAbilityDefinition* Definition = nullptr;
	return ResolveAbilityDefinition(Definition);
}

void UPDAbilitySourceComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Revoke(true);
	Super::EndPlay(EndPlayReason);
}

void UPDAbilitySourceComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDAbilitySourceComponent, BindingState);
}

UPDAbilitySystemComponent* UPDAbilitySourceComponent::GetGrantedAbilitySystem() const
{
	if (IsValid(AuthorityGrantedAbilitySystem))
	{
		return AuthorityGrantedAbilitySystem;
	}

	return BindingState.AbilitySystemOwner
		? Cast<UPDAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
				BindingState.AbilitySystemOwner))
		: nullptr;
}
