#include "PADO/AbilitySystem/Component/PDAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "PADO/AbilitySystem/Ability/PDGA_Base.h"
#include "PADO/AbilitySystem/Definition/PDAbilityDefinition.h"
#include "PADO/AbilitySystem/Definition/PDChannelActionDefinition.h"
#include "PADO/AbilitySystem/Definition/PDSingleActionDefinition.h"
#include "PADO/AbilitySystem/Interface/PDAbilitySourceInterface.h"
#include "PADO/AbilitySystem/Tag/PDAbilityGameplayTags.h"
#include "GameplayAbilitySpec.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDAbilitySystem, Log, All);

bool UPDAbilitySystemComponent::GetDefinitionFromSource(
	const UObject* SourceObject,
	const UPDAbilityDefinition*& OutDefinition,
	FString* OutError)
{
	OutDefinition = nullptr;

	const IPDAbilitySourceInterface* AbilitySource =
		Cast<IPDAbilitySourceInterface>(SourceObject);
	if (!AbilitySource)
	{
		if (OutError)
		{
			*OutError = TEXT("SourceObject가 PDAbilitySourceInterface를 구현하지 않습니다.");
		}
		return false;
	}

	if (!AbilitySource->ResolveAbilityDefinition(OutDefinition) ||
		!IsValid(OutDefinition))
	{
		if (OutError)
		{
			*OutError = TEXT("SourceObject가 Ability Definition을 제공하지 않았습니다.");
		}
		return false;
	}

	return true;
}

bool UPDAbilitySystemComponent::ResolveDefinitionFromSource(
	const UObject* SourceObject,
	const UPDAbilityDefinition*& OutDefinition,
	FString* OutError)
{
	if (!GetDefinitionFromSource(SourceObject, OutDefinition, OutError))
	{
		return false;
	}

	FString ValidationError;
	if (!OutDefinition->ValidateWithActionContract(ValidationError))
	{
		if (OutError)
		{
			*OutError = ValidationError;
		}
		return false;
	}

	return true;
}

FGameplayAbilitySpecHandle UPDAbilitySystemComponent::GrantAbilityFromSource(
	UObject* SourceObject,
	int32 InputId)
{
	// 검증은 GrantAbilityDefinition에서 정확히 한 번 수행한다.
	const UPDAbilityDefinition* Definition = nullptr;
	FString Error;
	if (!GetDefinitionFromSource(SourceObject, Definition, &Error))
	{
		UE_LOG(
			LogPDAbilitySystem,
			Warning,
			TEXT("Ability 부여를 거부했습니다. Source='%s', 이유: %s"),
			*GetNameSafe(SourceObject),
			*Error);
		return FGameplayAbilitySpecHandle();
	}

	return GrantAbilityDefinition(Definition, SourceObject, InputId);
}

FGameplayAbilitySpecHandle UPDAbilitySystemComponent::GrantAbilityDefinition(
	const UPDAbilityDefinition* Definition,
	UObject* SourceObject,
	int32 InputId)
{
	if (!IsOwnerActorAuthoritative() || !IsValid(Definition) ||
		!IsValid(SourceObject))
	{
		return FGameplayAbilitySpecHandle();
	}

	const UPDAbilityDefinition* SourceDefinition = nullptr;
	FString Error;
	if (!ResolveDefinitionFromSource(SourceObject, SourceDefinition, &Error) ||
		Definition != SourceDefinition)
	{
		UE_LOG(
			LogPDAbilitySystem,
			Warning,
			TEXT("Source에서 재구성한 Definition과 요청 Definition이 달라 부여를 거부했습니다. Source='%s', 이유: %s"),
			*GetNameSafe(SourceObject),
			*Error);
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& ExistingSpec : GetActivatableAbilities())
	{
		if (ExistingSpec.SourceObject.Get() != SourceObject)
		{
			continue;
		}

		const bool bSameGrant =
			ExistingSpec.Ability &&
			ExistingSpec.Ability->GetClass() == Definition->GetAbilityClass() &&
			ExistingSpec.Level == Definition->AbilityLevel &&
			ExistingSpec.InputID == InputId;
		if (bSameGrant)
		{
			return ExistingSpec.Handle;
		}

		UE_LOG(
			LogPDAbilitySystem,
			Warning,
			TEXT("같은 SourceObject에 서로 다른 Ability가 이미 부여되어 있습니다. Source='%s'"),
			*GetNameSafe(SourceObject));
		return FGameplayAbilitySpecHandle();
	}

	return GiveAbility(FGameplayAbilitySpec(
		Definition->GetAbilityClass(),
		Definition->AbilityLevel,
		InputId,
		SourceObject));
}

bool UPDAbilitySystemComponent::TryActivateGrantedAbility(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	return AbilityHandle.IsValid() && TryActivateAbility(AbilityHandle);
}

bool UPDAbilitySystemComponent::PressAbilityInputByHandle(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	if (IsOwnerActorAuthoritative())
	{
		return ProcessAbilityInputPressed(AbilityHandle);
	}

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const FGameplayTagContainer* AbilityTags = Spec && Spec->Ability
		? &Spec->Ability->GetAssetTags()
		: nullptr;
	if (AbilityTags && AbilityTags->HasTag(TAG_PD_Ability_Action) &&
		AreAbilityTagsBlocked(*AbilityTags))
	{
		return false;
	}

	// 이미 진행 중인 액션에 다시 들어온 Press는 여기서 버린다.
	// 서버는 활성 Spec의 재활성화를 거부하므로 이 입력은 어차피 아무것도
	// 시작하지 못한다. 그런데 예측 재생은 시작 전에 진행 중인 연출을 끊고
	// 처음부터 다시 트니, 유효한 연출만 망가뜨리고 되돌릴 방법이 없다.
	// 서버 RPC도 보내지 않아 Press와 Release 짝이 어긋나지 않게 한다.
	if (HasOutstandingLocalAction(AbilityHandle))
	{
		return false;
	}

	const uint32 ActionRequestId =
		BeginLocalActionMontagePrediction(AbilityHandle);
	ServerPressAbilityInputByHandle(AbilityHandle, ActionRequestId);
	return true;
}

bool UPDAbilitySystemComponent::ReleaseAbilityInputByHandle(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	if (IsOwnerActorAuthoritative())
	{
		return ProcessAbilityInputReleased(AbilityHandle);
	}

	if (LocalActionAbilityHandle == AbilityHandle)
	{
		bLocalActionInputReleased = true;
		if (bLocalActionStopOnRelease)
		{
			// Channel은 서버 왕복을 기다리지 않고 소유자 연출부터 멈춘다.
			StopLocalActionMontagePrediction(false);
		}
	}

	ServerReleaseAbilityInputByHandle(AbilityHandle);
	return true;
}

void UPDAbilitySystemComponent::ServerPressAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId)
{
	PendingServerActionRequests.Add(AbilityHandle, ActionRequestId);
	const bool bProcessed = ProcessAbilityInputPressed(AbilityHandle);
	PendingServerActionRequests.Remove(AbilityHandle);

	const uint32* ActiveRequestId =
		ActiveServerActionRequests.Find(AbilityHandle);
	if (!bProcessed || !ActiveRequestId ||
		*ActiveRequestId != ActionRequestId)
	{
		// 이미 활성인 Spec에 다시 들어온 Press와 활성화 거부 모두 예측 연출을 회수한다.
		ClientRejectActionMontage(AbilityHandle, ActionRequestId);
	}
}

void UPDAbilitySystemComponent::ServerReleaseAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	ProcessAbilityInputReleased(AbilityHandle);
}

bool UPDAbilitySystemComponent::ProcessAbilityInputPressed(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!IsOwnerActorAuthoritative() || !Spec)
	{
		return false;
	}

	AbilitySpecInputPressed(*Spec);
	return Spec->IsActive() || TryActivateAbility(AbilityHandle);
}

bool UPDAbilitySystemComponent::ProcessAbilityInputReleased(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!IsOwnerActorAuthoritative() || !Spec)
	{
		return false;
	}

	AbilitySpecInputReleased(*Spec);
	return true;
}

bool UPDAbilitySystemComponent::TryActivateGrantedAbilityWithEvent(
	FGameplayAbilitySpecHandle AbilityHandle,
	FGameplayTag EventTag,
	const FGameplayEventData& EventData)
{
	if (!AbilityHandle.IsValid() || !EventTag.IsValid() || !AbilityActorInfo.IsValid())
	{
		return false;
	}
	
	return TriggerAbilityFromGameplayEvent(
		AbilityHandle,
		AbilityActorInfo.Get(),
		EventTag,
		&EventData,
		*this);
}

bool UPDAbilitySystemComponent::RevokeAbilityByHandle(
	FGameplayAbilitySpecHandle AbilityHandle,
	bool bCancelActiveAbility)
{
	if (!IsOwnerActorAuthoritative() || !AbilityHandle.IsValid() ||
		!FindAbilitySpecFromHandle(AbilityHandle))
	{
		return false;
	}

	if (bCancelActiveAbility)
	{
		CancelAbilityHandle(AbilityHandle);
	}
	ClearAbility(AbilityHandle);
	return true;
}

FGameplayAbilitySpecHandle UPDAbilitySystemComponent::FindGrantedAbilityBySource(
	const UObject* SourceObject) const
{
	if (!IsValid(SourceObject))
	{
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.SourceObject.Get() == SourceObject)
		{
			return Spec.Handle;
		}
	}

	return FGameplayAbilitySpecHandle();
}

void UPDAbilitySystemComponent::PlayActionMontageForRemoteOwner(
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!AbilityHandle.IsValid() || !IsValid(Montage) ||
		!IsRemoteOwnerMontageTarget())
	{
		return;
	}

	const uint32* PendingRequestId =
		PendingServerActionRequests.Find(AbilityHandle);
	const uint32 ActionRequestId = PendingRequestId ? *PendingRequestId : 0;
	ActiveServerActionRequests.Add(AbilityHandle, ActionRequestId);
	ClientPlayActionMontage(
		AbilityHandle,
		ActionRequestId,
		Montage,
		PlayRate,
		StartSection);
}

void UPDAbilitySystemComponent::StopActionMontageForRemoteOwner(
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage* Montage)
{
	if (!AbilityHandle.IsValid() || !IsValid(Montage) ||
		!IsRemoteOwnerMontageTarget())
	{
		return;
	}

	const uint32* ActiveRequestId =
		ActiveServerActionRequests.Find(AbilityHandle);
	const uint32 ActionRequestId = ActiveRequestId ? *ActiveRequestId : 0;
	ClientStopActionMontage(AbilityHandle, ActionRequestId, Montage);
	ActiveServerActionRequests.Remove(AbilityHandle);
}

void UPDAbilitySystemComponent::ApplyActionMontageHitLagForRemoteOwner(
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage* Montage,
	float EffectivePlayRate,
	float Duration,
	uint32 HitLagGeneration)
{
	if (!AbilityHandle.IsValid() || !IsValid(Montage) ||
		!FMath::IsFinite(EffectivePlayRate) || EffectivePlayRate <= 0.0f ||
		!FMath::IsFinite(Duration) || Duration <= 0.0f ||
		!IsRemoteOwnerMontageTarget())
	{
		return;
	}

	const uint32* ActiveRequestId =
		ActiveServerActionRequests.Find(AbilityHandle);
	const uint32* PendingRequestId = ActiveRequestId
		? nullptr
		: PendingServerActionRequests.Find(AbilityHandle);
	ClientApplyActionMontageHitLag(
		AbilityHandle,
		ActiveRequestId
			? *ActiveRequestId
			: (PendingRequestId ? *PendingRequestId : 0),
		Montage,
		EffectivePlayRate,
		Duration,
		HitLagGeneration);
}

void UPDAbilitySystemComponent::ClientPlayActionMontage_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId,
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (MatchesLocalActionRequest(AbilityHandle, ActionRequestId))
	{
		const bool bSamePlayback =
			LocalActionMontage.Get() == Montage &&
			FMath::IsNearlyEqual(LocalActionPlayRate, PlayRate) &&
			LocalActionStartSection == StartSection;
		if (bSamePlayback && bLocalActionMontagePlayed)
		{
			// 로컬 선재생이 자연 종료됐더라도 서버 승인 시 다시 시작하지 않는다.
			return;
		}

		StopLocalActionMontagePrediction(false);
		LocalActionMontage = Montage;
		LocalActionPlayRate = PlayRate;
		LocalActionStartSection = StartSection;
		if (bLocalActionInputReleased && bLocalActionStopOnRelease)
		{
			return;
		}

		bLocalActionMontagePlayed =
			PlayActionMontageLocally(Montage, PlayRate, StartSection);
		return;
	}

	if (ActionRequestId != 0 && LocalActionRequestId != 0 &&
		static_cast<int32>(ActionRequestId - LocalActionRequestId) < 0)
	{
		// 이전 Press의 늦은 승인이 더 최신 로컬 연출을 덮어쓰지 않는다.
		return;
	}

	StopLocalActionMontagePrediction(true);
	LocalActionAbilityHandle = AbilityHandle;
	LocalActionRequestId = ActionRequestId;
	LocalActionMontage = Montage;
	LocalActionPlayRate = PlayRate;
	LocalActionStartSection = StartSection;
	UAnimMontage* ResolvedMontage = nullptr;
	float ResolvedPlayRate = 1.0f;
	FName ResolvedStartSection = NAME_None;
	ResolveLocalActionMontage(
		AbilityHandle,
		ResolvedMontage,
		ResolvedPlayRate,
		ResolvedStartSection,
		bLocalActionStopOnRelease);
	bLocalActionMontagePlayed =
		PlayActionMontageLocally(Montage, PlayRate, StartSection);
}

void UPDAbilitySystemComponent::ClientStopActionMontage_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId,
	UAnimMontage* Montage)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (ActionRequestId != 0 &&
		!MatchesLocalActionRequest(AbilityHandle, ActionRequestId))
	{
		return;
	}

	if (ActionRequestId == 0 && LocalActionRequestId != 0)
	{
		return;
	}

	StopMontageIfCurrent(*Montage);
	ResetLocalActionMontagePrediction();
}

void UPDAbilitySystemComponent::ClientRejectActionMontage_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId)
{
	if (MatchesLocalActionRequest(AbilityHandle, ActionRequestId))
	{
		StopLocalActionMontagePrediction(true);
	}
}

void UPDAbilitySystemComponent::ClientApplyActionMontageHitLag_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId,
	UAnimMontage* Montage,
	float EffectivePlayRate,
	float Duration,
	uint32 HitLagGeneration)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled() ||
		!MatchesLocalActionRequest(AbilityHandle, ActionRequestId) ||
		LocalActionMontage.Get() != Montage ||
		!FMath::IsFinite(EffectivePlayRate) || EffectivePlayRate <= 0.0f ||
		!FMath::IsFinite(Duration) || Duration <= 0.0f ||
		(HitLagGeneration != 0 && LocalActionHitLagGeneration != 0 &&
			static_cast<int32>(HitLagGeneration - LocalActionHitLagGeneration) <= 0))
	{
		return;
	}

	UAnimInstance* AnimInstance = AbilityActorInfo->GetAnimInstance();
	UWorld* World = GetWorld();
	if (!AnimInstance || !World || !AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	LocalActionHitLagGeneration = HitLagGeneration;
	AnimInstance->Montage_SetPlayRate(Montage, EffectivePlayRate);
	World->GetTimerManager().SetTimer(
		LocalActionHitLagTimerHandle,
		this,
		&UPDAbilitySystemComponent::RestoreLocalActionMontagePlayRate,
		Duration,
		false);
}

bool UPDAbilitySystemComponent::IsRemoteOwnerMontageTarget() const
{
	// 리슨 서버 호스트는 서버에서 이미 재생하므로 중복 재생을 막는다.
	return IsOwnerActorAuthoritative() && AbilityActorInfo.IsValid() &&
		!AbilityActorInfo->IsLocallyControlled();
}

uint32 UPDAbilitySystemComponent::BeginLocalActionMontagePrediction(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	do
	{
		++LastLocalActionRequestId;
	}
	while (LastLocalActionRequestId == 0);

	StopLocalActionMontagePrediction(true);
	LocalActionAbilityHandle = AbilityHandle;
	LocalActionRequestId = LastLocalActionRequestId;

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const UPDAbilityDefinition* Definition = nullptr;
	if (Spec && GetDefinitionFromSource(
		Spec->SourceObject.Get(), Definition))
	{
		const UPDSingleActionDefinition* SingleAction =
			Cast<UPDSingleActionDefinition>(Definition);
		if (SingleAction && SingleAction->ExecutesOnInputRelease())
		{
			// 충전 중에는 로컬 몽타주를 선재생하지 않는다. 서버가 Release를
			// 승인한 뒤 실제 실행 시점에 ClientPlayActionMontage로 시작한다.
			return LocalActionRequestId;
		}
	}

	UAnimMontage* Montage = nullptr;
	if (!ResolveLocalActionMontage(
		AbilityHandle,
		Montage,
		LocalActionPlayRate,
		LocalActionStartSection,
		bLocalActionStopOnRelease) ||
		!IsValid(Montage))
	{
		return LocalActionRequestId;
	}

	LocalActionMontage = Montage;
	bLocalActionMontagePlayed = PlayActionMontageLocally(
		Montage,
		LocalActionPlayRate,
		LocalActionStartSection);
	return LocalActionRequestId;
}

bool UPDAbilitySystemComponent::PlayActionMontageLocally(
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled() ||
		PlayMontageSimulated(Montage, PlayRate) <= 0.0f)
	{
		return false;
	}

	// PlayMontageSimulated는 StartSection을 사용하지 않으므로 직접 이동한다.
	if (!StartSection.IsNone())
	{
		if (UAnimInstance* AnimInstance = AbilityActorInfo->GetAnimInstance())
		{
			AnimInstance->Montage_JumpToSection(StartSection, Montage);
		}
	}
	return true;
}

void UPDAbilitySystemComponent::StopLocalActionMontagePrediction(
	bool bResetState)
{
	ClearLocalActionMontageHitLag();
	if (UAnimMontage* Montage = LocalActionMontage.Get())
	{
		StopMontageIfCurrent(*Montage);
	}
	bLocalActionMontagePlayed = false;

	if (bResetState)
	{
		ResetLocalActionMontagePrediction();
	}
}

void UPDAbilitySystemComponent::ResetLocalActionMontagePrediction()
{
	ClearLocalActionMontageHitLag();
	LocalActionAbilityHandle = FGameplayAbilitySpecHandle();
	LocalActionRequestId = 0;
	LocalActionMontage = nullptr;
	LocalActionPlayRate = 1.0f;
	LocalActionStartSection = NAME_None;
	bLocalActionStopOnRelease = false;
	bLocalActionInputReleased = false;
	bLocalActionMontagePlayed = false;
}

void UPDAbilitySystemComponent::RestoreLocalActionMontagePlayRate()
{
	LocalActionHitLagTimerHandle.Invalidate();
	UAnimMontage* Montage = LocalActionMontage.Get();
	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid()
		? AbilityActorInfo->GetAnimInstance()
		: nullptr;
	if (IsValid(Montage) && AnimInstance &&
		AnimInstance->Montage_IsPlaying(Montage))
	{
		AnimInstance->Montage_SetPlayRate(Montage, LocalActionPlayRate);
	}
}

void UPDAbilitySystemComponent::ClearLocalActionMontageHitLag()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocalActionHitLagTimerHandle);
	}
	else
	{
		LocalActionHitLagTimerHandle.Invalidate();
	}
	LocalActionHitLagGeneration = 0;
}

bool UPDAbilitySystemComponent::HasOutstandingLocalAction(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	// 서버가 확정한 활성 상태다. 한 왕복 늦게 도착하지만,
	// 로컬 연출이 먼저 끝나고 Ability는 아직 살아 있는 구간을 메운다.
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (Spec && Spec->IsActive())
	{
		return true;
	}

	if (LocalActionRequestId == 0 || LocalActionAbilityHandle != AbilityHandle)
	{
		return false;
	}

	// 아직 서버 응답 전이면 예측 연출이 살아 있는지로 판단한다.
	// 연출이 이미 끝났다면 응답이 유실돼도 다음 입력을 영구히 막지 않는다.
	const UAnimMontage* PredictedMontage = LocalActionMontage.Get();
	const UAnimInstance* AnimInstance = AbilityActorInfo.IsValid()
		? AbilityActorInfo->GetAnimInstance()
		: nullptr;
	return bLocalActionMontagePlayed && IsValid(PredictedMontage) &&
		AnimInstance && AnimInstance->Montage_IsPlaying(PredictedMontage);
}

bool UPDAbilitySystemComponent::MatchesLocalActionRequest(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId) const
{
	return LocalActionAbilityHandle == AbilityHandle &&
		LocalActionRequestId == ActionRequestId;
}

bool UPDAbilitySystemComponent::ResolveLocalActionMontage(
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage*& OutMontage,
	float& OutPlayRate,
	FName& OutStartSection,
	bool& bOutStopOnRelease) const
{
	OutMontage = nullptr;
	OutPlayRate = 1.0f;
	OutStartSection = NAME_None;
	bOutStopOnRelease = false;

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const UPDAbilityDefinition* Definition = nullptr;
	if (!Spec || !GetDefinitionFromSource(
		Spec->SourceObject.Get(), Definition) || !Definition)
	{
		return false;
	}

	OutMontage = Definition->ActionMontage.Montage;
	OutPlayRate = Definition->ActionMontage.PlayRate;
	OutStartSection = Definition->ActionMontage.StartSection;
	bOutStopOnRelease = Definition->IsA<UPDChannelActionDefinition>();
	return IsValid(OutMontage);
}
