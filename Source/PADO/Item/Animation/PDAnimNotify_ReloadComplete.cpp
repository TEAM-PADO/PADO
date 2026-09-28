#include "PADO/Item/Animation/PDAnimNotify_ReloadComplete.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Component/PDWeaponMagazineComponent.h"
#include "PADO/Item/PDWorldItemActor.h"

void UPDAnimNotify_ReloadComplete::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Holder = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Holder || !Holder->HasAuthority())
	{
		return;
	}

	const UPDHeldItemComponent* HeldItemComponent =
		Holder->FindComponentByClass<UPDHeldItemComponent>();
	APDWorldItemActor* Item =
		HeldItemComponent ? HeldItemComponent->GetHeldItem() : nullptr;
	if (UPDWeaponMagazineComponent* Magazine =
			Item ? Item->GetMagazineComponent() : nullptr)
	{
		Magazine->NotifyReloadComplete();
	}
}

FString UPDAnimNotify_ReloadComplete::GetNotifyName_Implementation() const
{
	return TEXT("Reload Complete");
}
