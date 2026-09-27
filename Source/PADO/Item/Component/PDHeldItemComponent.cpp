#include "PADO/Item/Component/PDHeldItemComponent.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/Interface/PDReloadableItem.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDHeldItemComponent, Log, All);

UPDHeldItemComponent::UPDHeldItemComponent()
{
	SetIsReplicatedByDefault(true);
}

bool UPDHeldItemComponent::ConfigureAttachment(
	USceneComponent* NewAttachmentComponent,
	FName NewHandSocketName)
{
	if (IsValid(HeldItem) || !IsValid(NewAttachmentComponent) ||
		NewAttachmentComponent->GetOwner() != GetOwner() ||
		NewHandSocketName.IsNone() ||
		!NewAttachmentComponent->DoesSocketExist(NewHandSocketName))
	{
		return false;
	}

	RuntimeAttachmentComponent = NewAttachmentComponent;
	HandSocketName = NewHandSocketName;
	return true;
}

void UPDHeldItemComponent::TryDropHeldItem()
{
	AActor* Holder = GetOwner();
	if (!Holder)
	{
		return;
	}

	if (Holder->HasAuthority())
	{
		DropHeldItemUsingSettings(FVector::ZeroVector);
	}
	else
	{
		ServerDropHeldItem();
	}
}

bool UPDHeldItemComponent::DropHeldItemUsingSettings(
	FVector AdditionalImpulse)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || !HasHeldItem())
	{
		return false;
	}

	return DropHeldItem(
		MakeHeldItemDropTransform(),
		Holder->GetActorForwardVector() * DropForwardImpulse +
			AdditionalImpulse);
}

bool UPDHeldItemComponent::TryPickUp(APDWorldItemActor* Item)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || !CanPickUpItem(Item))
	{
		return false;
	}

	const FName AttachSocket =
		ResolveHolderSocketName(Item->GetItemDefinition());
	USceneComponent* AttachComponent = ResolveAttachmentComponent(AttachSocket);
	if (!AttachComponent || !AttachComponent->DoesSocketExist(AttachSocket))
	{
		return false;
	}

	if (!Item->EnterHeldState(Holder, AttachComponent, AttachSocket))
	{
		return false;
	}

	HeldItem = Item;
	Holder->ForceNetUpdate();
	BroadcastHeldItemChanged();
	return true;
}

bool UPDHeldItemComponent::RequestPickUp(APDWorldItemActor* Item)
{
	AActor* Holder = GetOwner();
	if (!Holder || !IsValid(Item))
	{
		return false;
	}

	if (Holder->HasAuthority())
	{
		return TryPickUp(Item);
	}

	// 명백히 거부될 요청은 로컬에서 걸러 불필요한 RPC를 줄인다.
	// 복제 지연으로 로컬 판단이 틀릴 수 있으므로 확정은 서버가 한다.
	if (!CanPickUpItem(Item))
	{
		return false;
	}

	ServerPickUp(Item);
	return true;
}

void UPDHeldItemComponent::ServerPickUp_Implementation(APDWorldItemActor* Item)
{
	// TryPickUp이 권한, 현재 보유 상태, 대상의 World 상태와 Definition,
	// 거리, 손 소켓을 모두 서버 기준으로 다시 검증한다.
	TryPickUp(Item);
}

bool UPDHeldItemComponent::CanPickUpItem(const APDWorldItemActor* Item) const
{
	const AActor* Holder = GetOwner();
	return Holder &&
		!IsValid(HeldItem) &&
		IsValid(Item) &&
		Item->CanBePickedUp() &&
		MaxPickupDistance > 0.0f &&
		FVector::DistSquared(Holder->GetActorLocation(), Item->GetActorLocation()) <=
			FMath::Square(MaxPickupDistance);
}

bool UPDHeldItemComponent::DropHeldItem(
	const FTransform& DropTransform,
	FVector DropImpulse)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || !IsValid(HeldItem))
	{
		return false;
	}

	if (!HeldItem->ExitHeldState(DropTransform, DropImpulse))
	{
		return false;
	}

	HeldItem = nullptr;
	Holder->ForceNetUpdate();
	BroadcastHeldItemChanged();
	return true;
}

bool UPDHeldItemComponent::PressHeldItemUse()
{
	if (InputPressedItem.IsValid() || !IsValid(HeldItem))
	{
		return false;
	}

	InputPressedItem = HeldItem;
	if (!SendShot(HeldItem))
	{
		InputPressedItem = nullptr;
		InputPressedAbilityHandle = FGameplayAbilitySpecHandle();
		return false;
	}

	// 자동 발사는 누르고 있는 동안 활성화를 다시 연다. 한 발이 한 활성화라서
	// 발사마다 예측 ID가 생기고 연출과 반동을 그 단위로 걸 수 있다.
	const float FireInterval = ResolveAutomaticFireInterval(HeldItem);
	if (UWorld* World = GetWorld(); World && FireInterval > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			AutomaticFireTimerHandle,
			this,
			&UPDHeldItemComponent::HandleAutomaticFire,
			FireInterval,
			true);
	}

	return true;
}

bool UPDHeldItemComponent::ReleaseHeldItemUse()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutomaticFireTimerHandle);
	}
	else
	{
		AutomaticFireTimerHandle.Invalidate();
	}

	APDWorldItemActor* PressedItem = InputPressedItem.Get();
	const FGameplayAbilitySpecHandle PressedHandle = InputPressedAbilityHandle;
	InputPressedItem = nullptr;
	InputPressedAbilityHandle = FGameplayAbilitySpecHandle();
	return IsValid(PressedItem) && PressedItem->ReleaseUse(PressedHandle);
}

void UPDHeldItemComponent::HandleAutomaticFire()
{
	APDWorldItemActor* PressedItem = InputPressedItem.Get();

	// 발사 중에 아이템을 놓거나 바꿨으면 멈춘다.
	if (!IsValid(PressedItem) || PressedItem != HeldItem)
	{
		ReleaseHeldItemUse();
		return;
	}

	SendShot(PressedItem);
}

bool UPDHeldItemComponent::SendShot(APDWorldItemActor* Item)
{
	if (!IsValid(Item) || !Item->PressUse(InputPressedAbilityHandle))
	{
		return false;
	}

	// 서버가 탄약·재장전·쿨다운으로 거부할 수 있다. 여기서는 "보냈다"만 알린다.
	OnLocalShotFired.Broadcast(Item);
	return true;
}

float UPDHeldItemComponent::ResolveAutomaticFireInterval(
	const APDWorldItemActor* Item) const
{
	const UPDItemDefinition* Definition =
		IsValid(Item) ? Item->GetItemDefinition() : nullptr;
	const UPDSingleActionDefinition* SingleAction = Definition
		? Cast<UPDSingleActionDefinition>(Definition->UseAction)
		: nullptr;
	return SingleAction ? SingleAction->GetAutomaticFireInterval() : 0.0f;
}

bool UPDHeldItemComponent::TryReloadHeldItem()
{
	AActor* Holder = GetOwner();
	if (!Holder || !HasHeldItem())
	{
		return false;
	}

	if (Holder->HasAuthority())
	{
		return ReloadHeldItemAuthority();
	}

	ServerReloadHeldItem();
	return true;
}

bool UPDHeldItemComponent::UseHeldItemWithTarget(AActor* TargetActor)
{
	return GetOwner() && GetOwner()->HasAuthority() &&
		IsValid(HeldItem) &&
		HeldItem->ActivateUseWithTarget(TargetActor);
}

APDWorldItemActor* UPDHeldItemComponent::GetHeldItem() const
{
	// 파괴가 예약된 아이템이 GC 전까지 손에 남은 것처럼 보이지 않게 유효성까지 확인한다.
	return IsValid(HeldItem) ? HeldItem.Get() : nullptr;
}

bool UPDHeldItemComponent::HasHeldItem() const
{
	return GetHeldItem() != nullptr;
}

USceneComponent* UPDHeldItemComponent::GetAttachmentComponent() const
{
	return ResolveAttachmentComponent(HandSocketName);
}

FName UPDHeldItemComponent::GetHandSocketName() const
{
	return HandSocketName;
}

FName UPDHeldItemComponent::ResolveHolderSocketName(
	const UPDItemDefinition* ItemDefinition) const
{
	const FName SocketOverride = IsValid(ItemDefinition)
		? ItemDefinition->Presentation.HolderSocketNameOverride
		: NAME_None;
	return SocketOverride.IsNone() ? HandSocketName : SocketOverride;
}

USceneComponent* UPDHeldItemComponent::GetAttachmentComponentForItem(
	const UPDItemDefinition* ItemDefinition) const
{
	return ResolveAttachmentComponent(ResolveHolderSocketName(ItemDefinition));
}

void UPDHeldItemComponent::BeginPlay()
{
	Super::BeginPlay();

	RuntimeAttachmentComponent = ResolveAttachmentComponent(HandSocketName);
	if (!RuntimeAttachmentComponent || HandSocketName.IsNone() ||
		!RuntimeAttachmentComponent->DoesSocketExist(HandSocketName))
	{
		UE_LOG(
			LogPDHeldItemComponent,
			Warning,
			TEXT("Holder '%s'에서 Hand 소켓 '%s'를 제공하는 Attachment Component를 찾지 못했습니다."),
			*GetNameSafe(GetOwner()),
			*HandSocketName.ToString());
	}
}

void UPDHeldItemComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority() && IsValid(HeldItem))
	{
		HeldItem->ExitHeldState(HeldItem->GetActorTransform(), FVector::ZeroVector);
		HeldItem = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UPDHeldItemComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPDHeldItemComponent, HeldItem);
}

void UPDHeldItemComponent::OnRep_HeldItem()
{
	if (IsValid(HeldItem))
	{
		// RuntimeState와 HeldItem은 서로 다른 Actor/Component에서 복제되므로
		// 어느 쪽 RepNotify가 먼저 와도 클라이언트 부착을 다시 맞춘다.
		HeldItem->RefreshReplicatedAttachment();
	}
	BroadcastHeldItemChanged();
}

void UPDHeldItemComponent::ServerDropHeldItem_Implementation()
{
	DropHeldItemUsingSettings(FVector::ZeroVector);
}

void UPDHeldItemComponent::ServerReloadHeldItem_Implementation()
{
	ReloadHeldItemAuthority();
}

USceneComponent* UPDHeldItemComponent::ResolveAttachmentComponent(
	FName SocketName) const
{
	if (SocketName.IsNone())
	{
		return nullptr;
	}

	// 아이템이 소켓을 덮어쓰면 캐시된 컴포넌트가 그 소켓을 갖고 있지 않을 수 있다.
	if (IsValid(RuntimeAttachmentComponent) &&
		RuntimeAttachmentComponent->DoesSocketExist(SocketName))
	{
		return RuntimeAttachmentComponent;
	}

	AActor* Holder = GetOwner();
	if (!Holder)
	{
		return nullptr;
	}

	if (USceneComponent* ExplicitComponent =
		Cast<USceneComponent>(AttachmentComponent.GetComponent(Holder)))
	{
		// 비어 있는 FComponentReference는 Owner의 RootComponent를 반환할 수 있다.
		// 실제 소켓을 제공할 때만 명시적 설정으로 채택한다.
		if (ExplicitComponent->DoesSocketExist(SocketName))
		{
			return ExplicitComponent;
		}
	}

	// 별도 Configure 호출 없이 컴포넌트만 붙여도 동작하도록, 같은 이름의
	// 소켓을 가진 SkeletalMesh를 우선 탐색한다.
	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(Holder);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (IsValid(SkeletalMesh) && SkeletalMesh->DoesSocketExist(SocketName))
		{
			return SkeletalMesh;
		}
	}

	TInlineComponentArray<USceneComponent*> SceneComponents(Holder);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (IsValid(SceneComponent) &&
			SceneComponent->DoesSocketExist(SocketName))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

FTransform UPDHeldItemComponent::MakeHeldItemDropTransform() const
{
	const AActor* Holder = GetOwner();
	if (!Holder)
	{
		return FTransform::Identity;
	}

	const FVector DropLocation = Holder->GetActorLocation() +
		Holder->GetActorForwardVector() * DropForwardDistance +
		FVector::UpVector * DropHeightOffset;
	return FTransform(Holder->GetActorRotation(), DropLocation);
}

bool UPDHeldItemComponent::ClearHeldItemForDestruction(
	APDWorldItemActor* Item)
{
	AActor* Holder = GetOwner();
	if (!Holder || !Holder->HasAuthority() || !IsValid(Item) ||
		HeldItem != Item)
	{
		return false;
	}

	if (InputPressedItem.Get() == Item)
	{
		InputPressedItem = nullptr;
		InputPressedAbilityHandle = FGameplayAbilitySpecHandle();
	}

	if (!Item->ExitHeldState(Item->GetActorTransform(), FVector::ZeroVector))
	{
		UE_LOG(
			LogPDHeldItemComponent,
			Warning,
			TEXT("파괴되는 Item '%s'의 Held 상태 정리에 실패했지만 Holder 참조는 제거합니다."),
			*GetNameSafe(Item));
	}

	HeldItem = nullptr;
	Holder->ForceNetUpdate();
	BroadcastHeldItemChanged();
	return true;
}

bool UPDHeldItemComponent::ReloadHeldItemAuthority()
{
	AActor* Holder = GetOwner();
	APDWorldItemActor* Item = GetHeldItem();
	if (!Holder || !Holder->HasAuthority() || !Item ||
		!Item->GetClass()->ImplementsInterface(UPDReloadableItem::StaticClass()))
	{
		return false;
	}

	return IPDReloadableItem::Execute_TryStartReload(Item, Holder);
}

void UPDHeldItemComponent::BroadcastHeldItemChanged()
{
	OnHeldItemChanged.Broadcast(HeldItem);
}
