#include "PADO/AbilitySystem/Animation/PDAnimNotify_SendActionEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"

UPDAnimNotify_SendActionEvent::UPDAnimNotify_SendActionEvent()
{
	ActionEventTag = TAG_PD_GameplayEvent_Action_Execute;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 80, 60);

	// ASC가 없는 애님 에디터 프리뷰에서 Event 전송 실패 로그가 쌓이지 않게 한다.
	bShouldFireInEditor = false;
#endif
}

FString UPDAnimNotify_SendActionEvent::GetNotifyName_Implementation() const
{
	return ActionEventTag.IsValid()
		? ActionEventTag.ToString()
		: TEXT("PD Send Action Event");
}

void UPDAnimNotify_SendActionEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// 클라이언트 프록시에서도 Notify 자체는 호출되므로 Event 전송만 Authority로 막는다.
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() ||
		!ActionEventTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	UGameplayAbility* AnimatingAbility = AbilitySystem
		? AbilitySystem->GetAnimatingAbility()
		: nullptr;
	if (!IsValid(AnimatingAbility))
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = ActionEventTag;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;
	// 같은 ASC에서 기다리는 다른 Action이 이 Notify를 소비하지 못하게 한다.
	EventData.OptionalObject = AnimatingAbility;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		ActionEventTag,
		EventData);
}

#if WITH_EDITOR
bool UPDAnimNotify_SendActionEvent::CanBePlaced(
	UAnimSequenceBase* Animation) const
{
	// Action 실행 시점은 Ability가 재생하는 몽타주 타임라인에서만 의미가 있다.
	return Animation && Animation->IsA<UAnimMontage>();
}
#endif
