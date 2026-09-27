#include "PADO/Character/PDRecoilComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "PADO/AbilitySystem/Interface/PDAimStateProvider.h"
#include "PADO/Item/Component/PDHeldItemComponent.h"
#include "PADO/Item/Definition/PDItemDefinition.h"
#include "PADO/Item/PDWorldItemActor.h"
#include "PADO/Item/Trait/PDItemRecoilTrait.h"

UPDRecoilComponent::UPDRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

void UPDRecoilComponent::BeginPlay()
{
	Super::BeginPlay();

	// 반동은 컨트롤 회전 조작이라 로컬 조종자에게만 의미가 있다. 서버나
	// 시뮬레이션 프록시에서 돌면 남의 시점을 흔든다.
	if (!IsLocallyControlledOwner())
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (UPDHeldItemComponent* HeldItems =
			GetOwner()->FindComponentByClass<UPDHeldItemComponent>())
	{
		ShotFiredHandle = HeldItems->OnLocalShotFired.AddUObject(
			this,
			&UPDRecoilComponent::HandleLocalShotFired);
	}
}

void UPDRecoilComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPDHeldItemComponent* HeldItems =
			GetOwner() ? GetOwner()->FindComponentByClass<UPDHeldItemComponent>()
				: nullptr)
	{
		HeldItems->OnLocalShotFired.Remove(ShotFiredHandle);
	}

	ShotFiredHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPDRecoilComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastShot += DeltaTime;
	ApplyPendingRecoil(DeltaTime);
	RecoverPendingRecoil(DeltaTime);
}

void UPDRecoilComponent::HandleLocalShotFired(APDWorldItemActor* Item)
{
	const UPDItemDefinition* Definition =
		IsValid(Item) ? Item->GetItemDefinition() : nullptr;
	const UPDItemRecoilTrait* RecoilTrait =
		Definition ? Definition->FindTrait<UPDItemRecoilTrait>() : nullptr;
	if (!RecoilTrait)
	{
		return;
	}

	// 연사가 끊겼으면 스프레이 패턴을 처음부터 다시 센다.
	if (TimeSinceLastShot > RecoilTrait->RecoveryDelay)
	{
		ShotIndexInBurst = 0;
	}

	RecoveringTrait = RecoilTrait;

	const float StanceMultiplier = ResolveStanceMultiplier();
	const float SprayMultiplier = RecoilTrait->GetSprayMultiplier(ShotIndexInBurst);
	const float Pitch = (RecoilTrait->PitchPerShot +
		FMath::FRandRange(-RecoilTrait->PitchVariance, RecoilTrait->PitchVariance)) *
		SprayMultiplier * StanceMultiplier;
	const float Yaw = FMath::FRandRange(
		RecoilTrait->YawPerShotMin,
		RecoilTrait->YawPerShotMax) * SprayMultiplier * StanceMultiplier;

	PendingRecoil.X += Yaw;
	PendingRecoil.Y += FMath::Max(0.0f, Pitch);
	++ShotIndexInBurst;
	TimeSinceLastShot = 0.0f;
}

void UPDRecoilComponent::NotifyLookInput(const FVector2D& LookInput)
{
	// 유저가 반동을 아래로 눌러 상쇄했으면 그만큼은 복원 대상에서 뺀다.
	// 이걸 안 하면 유저 조작과 복원이 겹쳐 조준점이 과하게 내려간다.
	if (AppliedRecoil.Y <= 0.0f || LookInput.Y >= 0.0f)
	{
		return;
	}

	AppliedRecoil.Y = FMath::Max(0.0f, AppliedRecoil.Y + LookInput.Y);
}

bool UPDRecoilComponent::HasActiveRecoil() const
{
	return !PendingRecoil.IsNearlyZero() || !AppliedRecoil.IsNearlyZero();
}

bool UPDRecoilComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

APlayerController* UPDRecoilComponent::ResolveOwnerController() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

float UPDRecoilComponent::ResolveStanceMultiplier() const
{
	const IPDAimStateProvider* AimStateProvider =
		Cast<IPDAimStateProvider>(GetOwner());
	if (!AimStateProvider)
	{
		return IdleMultiplier;
	}

	switch (AimStateProvider->GetAimState())
	{
	case EPDAimState::Shouldered:
		return ShoulderedMultiplier;
	case EPDAimState::Aiming:
		return AimingMultiplier;
	default:
		return IdleMultiplier;
	}
}

void UPDRecoilComponent::ApplyPendingRecoil(float DeltaTime)
{
	APlayerController* Controller = ResolveOwnerController();
	if (!Controller || PendingRecoil.IsNearlyZero())
	{
		return;
	}

	// 한 프레임에 전부 실으면 스냅처럼 튄다. 남은 양을 조금씩 옮긴다.
	const float Alpha = FMath::Clamp(InterpSpeed * DeltaTime, 0.0f, 1.0f);
	const FVector2D Step = PendingRecoil * Alpha;
	PendingRecoil -= Step;

	// AddPitchInput은 아래가 양수라 위로 밀려면 부호를 뒤집는다.
	Controller->AddPitchInput(-Step.Y);
	Controller->AddYawInput(Step.X);

	// 복원하지 않는 무기는 밀린 조준점이 새 기준이다. 되돌릴 양을 쌓아 두면
	// 복원하는 무기로 바꿔 쏠 때 이 몫까지 끌어내린다.
	const UPDItemRecoilTrait* RecoilTrait = RecoveringTrait.Get();
	if (RecoilTrait && RecoilTrait->bEnableRecovery)
	{
		AppliedRecoil += Step;
	}
}

void UPDRecoilComponent::RecoverPendingRecoil(float DeltaTime)
{
	const UPDItemRecoilTrait* RecoilTrait = RecoveringTrait.Get();
	if (!RecoilTrait)
	{
		return;
	}

	// 복원을 끈 무기를 쏘면 앞 무기가 남긴 복원 몫도 버린다.
	if (!RecoilTrait->bEnableRecovery)
	{
		AppliedRecoil = FVector2D::ZeroVector;
	}

	if (AppliedRecoil.IsNearlyZero())
	{
		// 다 돌아왔으면 무기를 바꿔도 되도록 참조를 놓는다.
		AppliedRecoil = FVector2D::ZeroVector;
		RecoveringTrait.Reset();
		return;
	}

	if (TimeSinceLastShot < RecoilTrait->RecoveryDelay)
	{
		return;
	}

	APlayerController* Controller = ResolveOwnerController();
	if (!Controller)
	{
		return;
	}

	const float Alpha =
		FMath::Clamp(RecoilTrait->RecoverySpeed * DeltaTime, 0.0f, 1.0f);
	const FVector2D Step = AppliedRecoil * Alpha;
	AppliedRecoil -= Step;

	Controller->AddPitchInput(Step.Y);
	Controller->AddYawInput(-Step.X);
}
