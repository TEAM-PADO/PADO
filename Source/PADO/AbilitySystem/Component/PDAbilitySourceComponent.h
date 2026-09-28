#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "PADO/AbilitySystem/Interface/PDAbilitySourceInterface.h"
#include "PDAbilitySourceComponent.generated.h"

class AActor;
class UPDAbilityDefinition;
class UPDAbilitySystemComponent;

/** Handle과 대상 ASC Owner를 한 번에 복제하는 Source Binding 상태다. */
USTRUCT()
struct PADO_API FPDAbilitySourceBindingState
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilityHandle;

	UPROPERTY()
	TObjectPtr<AActor> AbilitySystemOwner;

	void Reset()
	{
		AbilityHandle = FGameplayAbilitySpecHandle();
		AbilitySystemOwner = nullptr;
	}
};

/** 아이템·함정 등 Actor에 붙여 동일한 Ability Source 수명주기를 제공한다. */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDAbilitySourceComponent
	: public UActorComponent
	, public IPDAbilitySourceInterface
{
	GENERATED_BODY()

public:
	UPDAbilitySourceComponent();

	virtual bool ResolveAbilityDefinition(
		const UPDAbilityDefinition*& OutDefinition) const override;

	/** Grant 중에는 Definition을 바꿀 수 없다. */
	bool ConfigureAbilityDefinition(UPDAbilityDefinition* NewDefinition);

	/**
	 * Owner 자신의 ASC에 저작된 Definition을 부여한다.
	 * 함정·AI·캐릭터 내재 능력처럼 스스로 능력을 갖는 소스의 공통 배선이다.
	 * 호출 전에 Owner의 ASC가 InitAbilityActorInfo를 마쳐야 한다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Ability")
	bool GrantToOwner();

	/** 아이템처럼 홀더의 ASC에 부여하는 소스가 사용한다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Ability")
	bool GrantToAbilitySystem(UPDAbilitySystemComponent* AbilitySystem);

	UFUNCTION(BlueprintCallable, Category = "PD|Ability")
	bool TryActivate();

	bool PressInput(FGameplayAbilitySpecHandle& OutPressedHandle);

	bool ReleaseInput(FGameplayAbilitySpecHandle PressedHandle);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Ability")
	bool TryActivateWithTarget(AActor* TargetActor);

	bool TryActivateWithHitResult(
		AActor* TargetActor,
		const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Ability")
	bool Revoke(bool bCancelActiveAbility = true);

	UFUNCTION(BlueprintPure, Category = "PD|Ability")
	FGameplayAbilitySpecHandle GetGrantedAbilityHandle() const;

	UFUNCTION(BlueprintPure, Category = "PD|Ability")
	bool HasAbilityDefinition() const;

	UFUNCTION(BlueprintPure, Category = "PD|Ability")
	UPDAbilityDefinition* GetActionDefinition() const;

	/**
	 * 이 컴포넌트에 직접 저작하는 Action Definition이다.
	 * 아이템처럼 상위 데이터에서 런타임에 주입하는 소스는 비워 둔다.
	 * 배치 인스턴스는 이 복제본이 아니라 Owner 클래스 기본값을 사용한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "PD|Ability")
	TObjectPtr<UPDAbilityDefinition> ActionDefinition;

	/**
	 * 배치 인스턴스 하나만 다른 행동을 써야 할 때 지정한다.
	 * 비어 있으면 BP 클래스 기본 ActionDefinition을 계속 따라간다.
	 */
	UPROPERTY(
		EditInstanceOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "PD|Ability|Instance Override")
	TObjectPtr<UPDAbilityDefinition> InstanceActionDefinition;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPDAbilityDefinition* ResolveAuthoredActionDefinition() const;
	bool TryActivateWithTargetInternal(
		AActor* TargetActor,
		const FHitResult* HitResult);
	bool CanActivateWithoutTarget() const;
	UPDAbilitySystemComponent* GetGrantedAbilitySystem() const;

	UPROPERTY(Transient)
	TObjectPtr<UPDAbilityDefinition> AbilityDefinition;

	UPROPERTY(Replicated)
	FPDAbilitySourceBindingState BindingState;

	UPROPERTY(Transient)
	TObjectPtr<UPDAbilitySystemComponent> AuthorityGrantedAbilitySystem;
};
