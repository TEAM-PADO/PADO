#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"
#include "GameplayAbilitySpecHandle.h"
#include "PADO/Interaction/Interface/PDInteractable.h"
#include "PADO/Item/Interface/PDReloadableItem.h"
#include "PDWorldItemActor.generated.h"

class UMeshComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UStaticMeshComponent;
class UPDAbilitySourceComponent;
class UPDHeldItemComponent;
class UPDItemDefinition;
class UPDWeaponMagazineComponent;

UENUM(BlueprintType)
enum class EPDWorldItemState : uint8
{
	World,
	Held
};

/** State와 Holder를 한 복제 단위로 유지한다. */
USTRUCT(BlueprintType)
struct PADO_API FPDWorldItemRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	EPDWorldItemState State = EPDWorldItemState::World;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	TObjectPtr<AActor> Holder;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	FVector_NetQuantize100 DropLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "PD|Item")
	FRotator DropRotation = FRotator::ZeroRotator;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FPDWorldItemStateChangedSignature,
	EPDWorldItemState,
	NewState,
	AActor*,
	NewHolder);

/** 인벤토리와 독립적으로 월드와 Holder 사이를 오가는 Ability Source Actor다. */
UCLASS(Blueprintable)
class PADO_API APDWorldItemActor
	: public AActor
	, public IPDReloadableItem
	, public IPDInteractable
{
	GENERATED_BODY()

public:
	APDWorldItemActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** 상호작용 주체가 지금 이 아이템을 집을 수 있는지다. 한 번에 하나만 든다. */
	virtual bool CanInteract_Implementation(
		const FPDInteractionContextStruct& Context) const override;

	/** 상호작용 주체가 이 아이템을 집는다. 서버에서만 불린다. */
	virtual bool Interact_Implementation(
		const FPDInteractionContextStruct& Context) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	virtual bool InitializeItem(UPDItemDefinition* NewDefinition);
	virtual bool TryStartReload_Implementation(AActor* RequestingHolder) override;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool CanBePickedUp() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	virtual bool IsUsable() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	EPDWorldItemState GetItemState() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	AActor* GetHolder() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	UPDItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	UMeshComponent* GetItemMesh() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	UPDAbilitySourceComponent* GetAbilitySourceComponent() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item|Magazine")
	UPDWeaponMagazineComponent* GetMagazineComponent() const;

	UPROPERTY(BlueprintAssignable, Category = "PD|Item")
	FPDWorldItemStateChangedSignature OnItemStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void HandleRuntimeItemStateChanged(
		EPDWorldItemState NewState,
		AActor* NewHolder);

	UFUNCTION()
	void OnRep_RuntimeState();

	UFUNCTION()
	void OnRep_ItemDefinition();

	/**
	 * 파생 Blueprint의 Class Defaults에서 기본 아이템을 정하고,
	 * 레벨에 배치한 인스턴스에서 필요하면 덮어쓴다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_ItemDefinition,
		Category = "PD|Item",
		meta = (ExposeOnSpawn = true))
	TObjectPtr<UPDItemDefinition> ItemDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	TObjectPtr<USphereComponent> ItemCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	TObjectPtr<UStaticMeshComponent> StaticItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item")
	TObjectPtr<USkeletalMeshComponent> SkeletalItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item|Action")
	TObjectPtr<UPDAbilitySourceComponent> AbilitySourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PD|Item|Magazine")
	TObjectPtr<UPDWeaponMagazineComponent> MagazineComponent;

private:
	friend class UPDHeldItemComponent;

	bool EnterHeldState(
		AActor* NewHolder,
		USceneComponent* AttachParent,
		FName AttachSocket);
	bool ExitHeldState(
		const FTransform& DropTransform,
		const FVector& DropImpulse);
	bool PressUse(FGameplayAbilitySpecHandle& OutPressedHandle);
	bool ReleaseUse(FGameplayAbilitySpecHandle PressedHandle);
	bool ActivateUseWithTarget(AActor* TargetActor);
	bool RefreshDefinition(FString* OutError = nullptr);
	void AlignGripToAttachmentSocket();
	void RefreshReplicatedAttachment();
	void ApplyStatePresentation();
	void BroadcastStateChanged();

	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	FPDWorldItemRuntimeState RuntimeState;

	EPDWorldItemState LastObservedState = EPDWorldItemState::World;
	bool bHasObservedState = false;
	bool bDefinitionValid = false;
};
