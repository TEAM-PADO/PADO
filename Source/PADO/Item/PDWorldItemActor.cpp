#include "PADO/Item/PDWorldItemActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "PADO/AbilitySystem/Component/PDAbilitySourceComponent.h"
#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/Trait/PDItemMagazineTrait.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDWorldItem, Log, All);

APDWorldItemActor::APDWorldItemActor()
{
	bReplicates = true;
	SetReplicateMovement(true);

	ItemCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ItemCollision"));
	SetRootComponent(ItemCollision);
	ItemCollision->InitSphereRadius(20.0f);
	ItemCollision->SetCollisionProfileName(TEXT("PhysicsActor"));
	ItemCollision->SetGenerateOverlapEvents(true);
	ItemCollision->SetSimulatePhysics(false);

	StaticItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticItemMesh"));
	StaticItemMesh->SetupAttachment(ItemCollision);
	StaticItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalItemMesh"));
	SkeletalItemMesh->SetupAttachment(ItemCollision);
	SkeletalItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AbilitySourceComponent =
		CreateDefaultSubobject<UPDAbilitySourceComponent>(TEXT("AbilitySource"));
	MagazineComponent =
		CreateDefaultSubobject<UPDWeaponMagazineComponent>(TEXT("ItemMagazine"));
}

void APDWorldItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDefinition();
	ApplyStatePresentation();
}

#if WITH_EDITOR
EDataValidationResult APDWorldItemActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// Definition이 비어 있는 상태는 막지 않는다. Blueprint 템플릿과 저작 중인
	// 인스턴스가 여기에 해당하고, 비어 있으면 CanBePickedUp()이 거부하므로
	// 게임 동작에 문제가 생기지 않는다. 지정된 Definition이 실제로 쓸 수
	// 없을 때만 오류로 처리한다.
	FString Error;
	if (ItemDefinition && !ItemDefinition->Validate(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}

	return Result == EDataValidationResult::NotValidated
		? EDataValidationResult::Valid
		: Result;
}
#endif

void APDWorldItemActor::BeginPlay()
{
	Super::BeginPlay();
	FString Error;
	if (!RefreshDefinition(&Error))
	{
		UE_LOG(
			LogPDWorldItem,
			Error,
			TEXT("Item '%s'의 Definition이 유효하지 않습니다: %s"),
			*GetName(),
			*Error);
	}
	if (HasAuthority() && MagazineComponent && bDefinitionValid)
	{
		MagazineComponent->InitializeMagazine(false);
	}
	ApplyStatePresentation();
}

void APDWorldItemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 들린 채로 파괴되면 Holder의 참조와 Ability를 여기서 정리한다.
	// 이게 없으면 HeldItem이 무효 포인터로 남아 OnHeldItemChanged가 울리지 않는다.
	if (HasAuthority() && RuntimeState.State == EPDWorldItemState::Held &&
		IsValid(RuntimeState.Holder))
	{
		if (UPDHeldItemComponent* HolderItems =
			RuntimeState.Holder->FindComponentByClass<UPDHeldItemComponent>())
		{
			HolderItems->ClearHeldItemForDestruction(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool APDWorldItemActor::InitializeItem(UPDItemDefinition* NewDefinition)
{
	if (!HasAuthority() || RuntimeState.State != EPDWorldItemState::World ||
		!IsValid(NewDefinition))
	{
		return false;
	}

	UPDItemDefinition* PreviousDefinition = ItemDefinition;
	ItemDefinition = NewDefinition;
	FString Error;
	if (!RefreshDefinition(&Error))
	{
		ItemDefinition = PreviousDefinition;
		RefreshDefinition();
		return false;
	}

	if (!MagazineComponent || !MagazineComponent->InitializeMagazine(true))
	{
		ItemDefinition = PreviousDefinition;
		RefreshDefinition();
		return false;
	}
	ApplyStatePresentation();
	ForceNetUpdate();
	return true;
}

bool APDWorldItemActor::TryStartReload_Implementation(AActor* RequestingHolder)
{
	return HasAuthority() && IsValid(RequestingHolder) &&
		RuntimeState.State == EPDWorldItemState::Held &&
		RuntimeState.Holder == RequestingHolder &&
		MagazineComponent && MagazineComponent->TryStartReload();
}

bool APDWorldItemActor::EnterHeldState(
	AActor* NewHolder,
	USceneComponent* AttachParent,
	FName AttachSocket)
{
	if (!HasAuthority() || !CanBePickedUp() || !IsValid(NewHolder) ||
		!IsValid(AttachParent) || AttachParent->GetOwner() != NewHolder ||
		AttachSocket.IsNone() || !AttachParent->DoesSocketExist(AttachSocket))
	{
		return false;
	}

	ItemCollision->SetSimulatePhysics(false);
	ItemCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!AttachToComponent(
		AttachParent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocket))
	{
		ApplyStatePresentation();
		return false;
	}

	AlignGripToAttachmentSocket();
	RuntimeState.Holder = NewHolder;
	RuntimeState.State = EPDWorldItemState::Held;
	SetOwner(NewHolder);

	if (IsUsable())
	{
		UPDAbilitySystemComponent* HolderAbilitySystem =
			Cast<UPDAbilitySystemComponent>(
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewHolder));
		if (!HolderAbilitySystem ||
			!AbilitySourceComponent->GrantToAbilitySystem(HolderAbilitySystem))
		{
			UE_LOG(
				LogPDWorldItem,
				Warning,
				TEXT("Holder '%s'에 PD ASC가 없거나 Item '%s'의 Ability 부여에 실패했습니다."),
				*GetNameSafe(NewHolder),
				*GetName());
		}
	}

	ForceNetUpdate();
	BroadcastStateChanged();
	return true;
}

bool APDWorldItemActor::ExitHeldState(
	const FTransform& DropTransform,
	const FVector& DropImpulse)
{
	if (!HasAuthority() || RuntimeState.State != EPDWorldItemState::Held)
	{
		return false;
	}

	if (!AbilitySourceComponent->Revoke(true))
	{
		UE_LOG(
			LogPDWorldItem,
			Warning,
			TEXT("Item '%s'의 Ability 회수에 실패했지만 드롭은 진행합니다."),
			*GetName());
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	RuntimeState.Holder = nullptr;
	RuntimeState.State = EPDWorldItemState::World;
	RuntimeState.DropLocation = DropTransform.GetLocation();
	RuntimeState.DropRotation = DropTransform.Rotator();
	SetActorTransform(DropTransform, false, nullptr, ETeleportType::TeleportPhysics);
	ApplyStatePresentation();

	if (!DropImpulse.IsNearlyZero() && ItemCollision->IsSimulatingPhysics())
	{
		ItemCollision->AddImpulse(DropImpulse, NAME_None, true);
	}

	ForceNetUpdate();
	BroadcastStateChanged();
	return true;
}

bool APDWorldItemActor::PressUse(FGameplayAbilitySpecHandle& OutPressedHandle)
{
	return RuntimeState.State == EPDWorldItemState::Held &&
		IsUsable() &&
		AbilitySourceComponent->PressInput(OutPressedHandle);
}

bool APDWorldItemActor::ReleaseUse(FGameplayAbilitySpecHandle PressedHandle)
{
	return IsUsable() && AbilitySourceComponent->ReleaseInput(PressedHandle);
}

bool APDWorldItemActor::ActivateUseWithTarget(AActor* TargetActor)
{
	return HasAuthority() && RuntimeState.State == EPDWorldItemState::Held &&
		IsUsable() && AbilitySourceComponent->TryActivateWithTarget(TargetActor);
}

bool APDWorldItemActor::CanBePickedUp() const
{
	return RuntimeState.State == EPDWorldItemState::World && bDefinitionValid;
}

bool APDWorldItemActor::IsUsable() const
{
	return bDefinitionValid && ItemDefinition && ItemDefinition->IsUsable();
}

EPDWorldItemState APDWorldItemActor::GetItemState() const
{
	return RuntimeState.State;
}

AActor* APDWorldItemActor::GetHolder() const
{
	return RuntimeState.Holder;
}

UPDItemDefinition* APDWorldItemActor::GetItemDefinition() const
{
	return ItemDefinition;
}

UMeshComponent* APDWorldItemActor::GetItemMesh() const
{
	if (!ItemDefinition)
	{
		return nullptr;
	}
	return ItemDefinition->Presentation.SkeletalMesh
		? static_cast<UMeshComponent*>(SkeletalItemMesh)
		: static_cast<UMeshComponent*>(StaticItemMesh);
}

UPDAbilitySourceComponent* APDWorldItemActor::GetAbilitySourceComponent() const
{
	return AbilitySourceComponent;
}

UPDWeaponMagazineComponent* APDWorldItemActor::GetMagazineComponent() const
{
	// 탄창 Trait이 없는 아이템은 Component가 있어도 노출하지 않는다.
	return ItemDefinition && ItemDefinition->FindTrait<UPDItemMagazineTrait>()
		? MagazineComponent
		: nullptr;
}

bool APDWorldItemActor::RefreshDefinition(FString* OutError)
{
	bDefinitionValid = false;
	if (!ItemDefinition)
	{
		StaticItemMesh->SetStaticMesh(nullptr);
		SkeletalItemMesh->SetSkeletalMesh(nullptr);
		AbilitySourceComponent->ConfigureAbilityDefinition(nullptr);
		if (OutError)
		{
			*OutError = TEXT("ItemDefinition이 비어 있습니다.");
		}
		return false;
	}

	FString Error;
	if (!ItemDefinition->Validate(Error))
	{
		if (OutError)
		{
			*OutError = MoveTemp(Error);
		}
		return false;
	}

	const FPDItemPresentationStruct& Presentation = ItemDefinition->Presentation;
	StaticItemMesh->SetStaticMesh(Presentation.StaticMesh);
	SkeletalItemMesh->SetSkeletalMesh(Presentation.SkeletalMesh);
	StaticItemMesh->SetRelativeTransform(Presentation.MeshRelativeTransform);
	SkeletalItemMesh->SetRelativeTransform(Presentation.MeshRelativeTransform);
	StaticItemMesh->SetVisibility(IsValid(Presentation.StaticMesh), true);
	SkeletalItemMesh->SetVisibility(IsValid(Presentation.SkeletalMesh), true);
	ItemCollision->SetSphereRadius(Presentation.CollisionRadius);
	if (!AbilitySourceComponent->ConfigureAbilityDefinition(ItemDefinition->UseAction))
	{
		if (OutError)
		{
			*OutError = TEXT("Grant 중인 Ability Definition은 교체할 수 없습니다.");
		}
		return false;
	}

	bDefinitionValid = true;
	if (OutError)
	{
		OutError->Reset();
	}
	return true;
}

void APDWorldItemActor::AlignGripToAttachmentSocket()
{
	UMeshComponent* Mesh = GetItemMesh();
	const FName GripSocket = ItemDefinition
		? ItemDefinition->Presentation.GripSocketName
		: NAME_None;
	if (!Mesh || GripSocket.IsNone() || !Mesh->DoesSocketExist(GripSocket))
	{
		SetActorRelativeTransform(FTransform::Identity);
		return;
	}

	SetActorRelativeTransform(
		Mesh->GetSocketTransform(GripSocket, RTS_Actor).Inverse());
}

void APDWorldItemActor::RefreshReplicatedAttachment()
{
	if (HasAuthority())
	{
		return;
	}

	if (RuntimeState.State == EPDWorldItemState::World)
	{
		if (GetRootComponent() && GetRootComponent()->GetAttachParent())
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		SetActorLocationAndRotation(
			RuntimeState.DropLocation,
			RuntimeState.DropRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		return;
	}

	if (!IsValid(RuntimeState.Holder))
	{
		return;
	}

	UPDHeldItemComponent* HolderItems =
		RuntimeState.Holder->FindComponentByClass<UPDHeldItemComponent>();
	const FName AttachSocket = HolderItems
		? HolderItems->ResolveHolderSocketName(ItemDefinition)
		: NAME_None;
	USceneComponent* AttachParent = HolderItems
		? HolderItems->GetAttachmentComponentForItem(ItemDefinition)
		: nullptr;
	if (!AttachParent || AttachSocket.IsNone() ||
		!AttachParent->DoesSocketExist(AttachSocket))
	{
		return;
	}

	if (GetRootComponent()->GetAttachParent() != AttachParent ||
		GetAttachParentSocketName() != AttachSocket)
	{
		AttachToComponent(
			AttachParent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocket);
	}
	AlignGripToAttachmentSocket();
}

void APDWorldItemActor::ApplyStatePresentation()
{
	if (!bDefinitionValid || !ItemDefinition)
	{
		ItemCollision->SetSimulatePhysics(false);
		ItemCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	if (RuntimeState.State == EPDWorldItemState::Held)
	{
		ItemCollision->SetSimulatePhysics(false);
		ItemCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	ItemCollision->SetCollisionProfileName(
		ItemDefinition->Presentation.WorldCollisionProfile);
	ItemCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemCollision->SetSimulatePhysics(
		ItemDefinition->Presentation.bSimulatePhysicsInWorld);
}

void APDWorldItemActor::OnRep_RuntimeState()
{
	RefreshReplicatedAttachment();
	ApplyStatePresentation();

	// 구조체 안의 다른 필드만 바뀌어도 RepNotify는 다시 온다. Holder는 언제나
	// State 전이와 함께 바뀌므로, 상태가 실제로 달라졌을 때만 알린다.
	const bool bStateChanged =
		!bHasObservedState || LastObservedState != RuntimeState.State;
	LastObservedState = RuntimeState.State;
	bHasObservedState = true;
	if (bStateChanged)
	{
		BroadcastStateChanged();
	}
}

void APDWorldItemActor::OnRep_ItemDefinition()
{
	RefreshDefinition();
	RefreshReplicatedAttachment();
	ApplyStatePresentation();
}

void APDWorldItemActor::BroadcastStateChanged()
{
	HandleRuntimeItemStateChanged(RuntimeState.State, RuntimeState.Holder);
	OnItemStateChanged.Broadcast(RuntimeState.State, RuntimeState.Holder);
}

void APDWorldItemActor::HandleRuntimeItemStateChanged(
	EPDWorldItemState NewState,
	AActor* NewHolder)
{
	if (HasAuthority() && NewState != EPDWorldItemState::Held &&
		MagazineComponent)
	{
		MagazineComponent->CancelReload();
	}
}

void APDWorldItemActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APDWorldItemActor, ItemDefinition);
	DOREPLIFETIME(APDWorldItemActor, RuntimeState);
}
