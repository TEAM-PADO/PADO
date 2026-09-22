#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "PDHeldItemComponent.generated.h"

class APDWorldItemActor;
class UPDItemDefinition;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPDHeldItemChangedSignature,
	APDWorldItemActor*,
	NewHeldItem);

/** 한 Actor가 한 번에 하나의 월드 아이템만 들도록 보장하는 Holder 측 컴포넌트다. */
UCLASS(BlueprintType, ClassGroup = (PD), meta = (BlueprintSpawnableComponent))
class PADO_API UPDHeldItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPDHeldItemComponent();

	/** 런타임에 기본 AttachmentComponent 설정을 덮어써야 할 때 사용한다. */
	bool ConfigureAttachment(
		USceneComponent* NewAttachmentComponent,
		FName NewHandSocketName);

	/** 입력 계층에서 호출한다. 서버가 컴포넌트 설정으로 Drop Transform을 계산한다. */
	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	void TryDropHeldItem();

	/** 서버에서 컴포넌트 설정을 사용하고 추가 Impulse를 더해 현재 아이템을 드롭한다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool DropHeldItemUsingSettings(FVector AdditionalImpulse);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool TryPickUp(APDWorldItemActor* Item);

	/**
	 * 입력 계층에서 호출한다. 로컬이 고른 대상을 서버로 보내 줍기를 요청하고,
	 * 서버가 권한·보유 상태·대상 상태·거리·손 소켓을 다시 검증해 확정한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool RequestPickUp(APDWorldItemActor* Item);

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool CanPickUpItem(const APDWorldItemActor* Item) const;

	/** 서버가 줍기를 허용하는 캐릭터 기준 최대 거리다. */
	UFUNCTION(BlueprintPure, Category = "PD|Item|Pickup")
	float GetMaxPickupDistance() const { return MaxPickupDistance; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool DropHeldItem(
		const FTransform& DropTransform,
		FVector DropImpulse = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool PressHeldItemUse();

	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool ReleaseHeldItemUse();

	/** 로컬 입력 의도를 서버로 보내 현재 Reloadable Held Item의 재장전을 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool TryReloadHeldItem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool UseHeldItemWithTarget(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	APDWorldItemActor* GetHeldItem() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool HasHeldItem() const;

	/** 명시적 설정 또는 HandSocketName을 가진 Owner 컴포넌트를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "PD|Item|Attachment")
	USceneComponent* GetAttachmentComponent() const;

	UFUNCTION(BlueprintPure, Category = "PD|Item|Attachment")
	FName GetHandSocketName() const;

	/**
	 * 아이템이 Presentation에서 소켓을 덮어썼으면 그 이름을, 아니면 HandSocketName을
	 * 반환한다. Definition이 없으면 언제나 HandSocketName이다.
	 */
	UFUNCTION(BlueprintPure, Category = "PD|Item|Attachment")
	FName ResolveHolderSocketName(const UPDItemDefinition* ItemDefinition) const;

	/** 아이템이 덮어쓴 소켓까지 반영해 부착 대상 컴포넌트를 찾는다. */
	UFUNCTION(BlueprintPure, Category = "PD|Item|Attachment")
	USceneComponent* GetAttachmentComponentForItem(
		const UPDItemDefinition* ItemDefinition) const;

	UPROPERTY(BlueprintAssignable, Category = "PD|Item")
	FPDHeldItemChangedSignature OnHeldItemChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void ServerPickUp(APDWorldItemActor* Item);

	UFUNCTION(Server, Reliable)
	void ServerDropHeldItem();

	UFUNCTION(Server, Reliable)
	void ServerReloadHeldItem();

	UFUNCTION()
	void OnRep_HeldItem();

	/**
	 * Holder의 SkeletalMesh 등 손 소켓을 제공하는 컴포넌트다.
	 * 비어 있으면 Owner에서 HandSocketName을 가진 Scene Component를 자동 탐색한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Item|Attachment")
	FComponentReference AttachmentComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Item|Attachment")
	FName HandSocketName = TEXT("HandItem");

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Item|Pickup",
		meta = (ClampMin = "0.0"))
	float MaxPickupDistance = 250.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Item|Drop",
		meta = (ClampMin = "0.0"))
	float DropForwardDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PD|Item|Drop")
	float DropHeightOffset = 30.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "PD|Item|Drop",
		meta = (ClampMin = "0.0"))
	float DropForwardImpulse = 0.0f;

private:
	friend class APDWorldItemActor;

	USceneComponent* ResolveAttachmentComponent(FName SocketName) const;
	FTransform MakeHeldItemDropTransform() const;
	bool ClearHeldItemForDestruction(APDWorldItemActor* Item);
	bool ReloadHeldItemAuthority();
	void BroadcastHeldItemChanged();

	UPROPERTY(ReplicatedUsing = OnRep_HeldItem)
	TObjectPtr<APDWorldItemActor> HeldItem;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RuntimeAttachmentComponent;

	/** Press 당시의 아이템이다. Release가 이후에 든 다른 아이템으로 새지 않게 한다. */
	UPROPERTY(Transient)
	TWeakObjectPtr<APDWorldItemActor> InputPressedItem;

	FGameplayAbilitySpecHandle InputPressedAbilityHandle;
};
