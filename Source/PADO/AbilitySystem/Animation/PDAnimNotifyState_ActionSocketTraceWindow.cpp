#include "PADO/AbilitySystem/Animation/PDAnimNotifyState_ActionSocketTraceWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Ability/PDGA_ActionRuntimeBase.h"

namespace PDActionSocketTraceNotify
{
	UPDGA_ActionRuntimeBase* ResolveAbility(USkeletalMeshComponent* MeshComp)
	{
		AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
		if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
		{
			return nullptr;
		}

		UAbilitySystemComponent* AbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
		return AbilitySystem
			? Cast<UPDGA_ActionRuntimeBase>(AbilitySystem->GetAnimatingAbility())
			: nullptr;
	}
}

UPDAnimNotifyState_ActionSocketTraceWindow::
UPDAnimNotifyState_ActionSocketTraceWindow()
{
	NotifyStateBehaviorFlags =
		static_cast<uint8>(EAnimNotifyStateBehaviorFlags::NoMergeOnConcurrentPlay);

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(70, 180, 235);
	bShouldFireInEditor = false;
#endif
}

FString UPDAnimNotifyState_ActionSocketTraceWindow::
GetNotifyName_Implementation() const
{
	return TEXT("PD Action Socket Trace Window");
}

void UPDAnimNotifyState_ActionSocketTraceWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UPDGA_ActionRuntimeBase* Ability =
		PDActionSocketTraceNotify::ResolveAbility(MeshComp))
	{
		Ability->NotifySocketTraceWindowBegin();
	}
}

void UPDAnimNotifyState_ActionSocketTraceWindow::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (UPDGA_ActionRuntimeBase* Ability =
		PDActionSocketTraceNotify::ResolveAbility(MeshComp))
	{
		Ability->NotifySocketTraceWindowTick();
	}
}

void UPDAnimNotifyState_ActionSocketTraceWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UPDGA_ActionRuntimeBase* Ability =
		PDActionSocketTraceNotify::ResolveAbility(MeshComp))
	{
		Ability->NotifySocketTraceWindowEnd();
	}
}

#if WITH_EDITOR
bool UPDAnimNotifyState_ActionSocketTraceWindow::CanBePlaced(
	UAnimSequenceBase* Animation) const
{
	return Animation && Animation->IsA<UAnimMontage>();
}
#endif
