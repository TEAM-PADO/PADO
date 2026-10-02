#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "PDHeldItemComponent.generated.h"

class APDWorldItemActor;
class UPDItemDefinition;
class UPDSingleActionDefinition;
class USceneComponent;

/** 자동 발사가 한 발 나갈 때마다 로컬에서 브로드캐스트한다. 반동·연출이 구독한다. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	FPDHeldItemFiredSignature,
	APDWorldItemActor*);

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

	/**
	 * 서버에서 줍기를 확정한다. 보유 상태, 대상 상태, 손 소켓을 본다.
	 * 입력에서 오는 줍기는 상호작용 경로를 거친다. 손 닿는 거리는 대상을 고른
	 * 상호작용 컴포넌트가 판단하므로 여기서 재지 않는다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool TryPickUp(APDWorldItemActor* Item);

	/** 지금 이 아이템을 집을 수 있는 상태인지다. 한 번에 하나만 들고, 손을 쓸 수 없으면 집지 않는다. */
	UFUNCTION(BlueprintPure, Category = "PD|Item")
	bool CanPickUpItem(const APDWorldItemActor* Item) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool DropHeldItem(
		const FTransform& DropTransform,
		FVector DropImpulse = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool PressHeldItemUse();

	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool ReleaseHeldItemUse();

	/**
	 * 이 머신에서 한 발이 실제로 나간 순간 발생한다. 서버 확정을 기다리지 않는다.
	 *
	 * Single Action은 Press로 활성화된 순간이 한 발이다. Fire Action은 Press가
	 * 발이 아니므로 Action이 발마다 NotifyLocalShotFired로 알린다.
	 */
	FPDHeldItemFiredSignature OnLocalShotFired;

	/** 발을 쏜 Action이 부른다. 반동처럼 발마다 반응하는 쪽이 OnLocalShotFired로 받는다. */
	void NotifyLocalShotFired(APDWorldItemActor* Item);

	/**
	 * 로컬 입력 의도를 서버로 보내 현재 Reloadable Held Item의 재장전을 요청한다.
	 *
	 * 소유 클라이언트는 예측 탄약으로 재장전이 가능할 때만 보내고, 보낸 순간부터
	 * 서버의 답이 올 때까지 발사를 멈춘다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	bool TryReloadHeldItem();

	/** 서버에서 현재 Held Item의 재장전을 취소한다. 진행 중이던 재장전이 있었으면 true다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PD|Item")
	bool CancelHeldItemReload();

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
	void ServerDropHeldItem();

	UFUNCTION(Server, Reliable)
	void ServerReloadHeldItem();

	/**
	 * 서버가 재장전 요청을 거부했다. 알리지 않으면 소유 클라이언트는 요청을 보낸
	 * 뒤 멈춘 발사가 풀리지 않는다.
	 */
	UFUNCTION(Client, Reliable)
	void ClientRejectReload();

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

	/**
	 * Holder가 손을 쓸 수 없는 상태인가(State.HandsBlocked). 입력에서 온 재장전,
	 * 드롭, 줍기를 거부한다. 탑승 직전에 보낸 요청이 앉은 뒤에 도착해도 손을 쓰지 않게 한다.
	 */
	bool AreHandsBlocked() const;

	UPROPERTY(ReplicatedUsing = OnRep_HeldItem)
	TObjectPtr<APDWorldItemActor> HeldItem;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RuntimeAttachmentComponent;

	/** 자동 발사 무기를 누르고 있는 동안 간격마다 다시 쏜다. */
	void HandleAutomaticFire();
	bool SendShot(APDWorldItemActor* Item);

	/** Press로 활성화된 순간이 곧 한 발인가. Fire Action처럼 발을 따로 세면 아니다. */
	bool DoesPressFireShot(const APDWorldItemActor* Item) const;

	/** 자동이 아니면 0을 돌려준다. */
	float ResolveAutomaticFireInterval(const APDWorldItemActor* Item) const;

	FTimerHandle AutomaticFireTimerHandle;

	/** Press 당시의 아이템이다. Release가 이후에 든 다른 아이템으로 새지 않게 한다. */
	UPROPERTY(Transient)
	TWeakObjectPtr<APDWorldItemActor> InputPressedItem;

	FGameplayAbilitySpecHandle InputPressedAbilityHandle;

	/** 재장전을 요청한 아이템이다. 서버가 거부하면 이 아이템의 요청 상태를 푼다. */
	TWeakObjectPtr<APDWorldItemActor> ReloadRequestedItem;
};
