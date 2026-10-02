#include "PADO/Character/PDRespawnComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "PADO/Character/PDCharacterBase.h"
#include "PADO/Character/PDHealthComponent.h"
#include "PADO/Character/PDPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPDRespawn, Log, All);

UPDRespawnComponent::UPDRespawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPDRespawnComponent::RespawnAt(const FTransform& SpawnTransform)
{
	AController* Controller = Cast<AController>(GetOwner());
	UWorld* World = GetWorld();
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	if (!Controller || !Controller->HasAuthority() || !GameMode)
	{
		return false;
	}

	APawn* OldPawn = Controller->GetPawn();
	if (const APDCharacterBase* OldCharacter = Cast<APDCharacterBase>(OldPawn);
		OldCharacter && OldCharacter->IsAlive())
	{
		return false;
	}

	World->GetTimerManager().ClearTimer(RespawnTimerHandle);

	// 시체를 정리한다. 엔진은 컨트롤러가 몸을 쥐고 있으면 새로 만들지 않고 그 몸을 다시 쓴다.
	if (OldPawn)
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	if (APDPlayerState* PlayerState = Controller->GetPlayerState<APDPlayerState>())
	{
		PlayerState->ResetForRespawn();
	}

	GameMode->RestartPlayerAtTransform(Controller, SpawnTransform);
	if (!Controller->GetPawn())
	{
		// 그 자리를 다른 플레이어나 탈것이 막아 스폰하지 못했다.
		UE_LOG(
			LogPDRespawn,
			Warning,
			TEXT("%s: %s에 스폰하지 못해 기본 시작 지점에서 다시 시작한다."),
			*GetNameSafe(Controller),
			*SpawnTransform.GetLocation().ToString());
		GameMode->RestartPlayer(Controller);
	}

	return Controller->GetPawn() != nullptr;
}

bool UPDRespawnComponent::IsRespawnPending() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(RespawnTimerHandle);
}

void UPDRespawnComponent::BeginPlay()
{
	Super::BeginPlay();

	AController* Controller = Cast<AController>(GetOwner());
	if (!Controller || !Controller->HasAuthority())
	{
		return;
	}

	Controller->OnPossessedPawnChanged.AddDynamic(
		this,
		&UPDRespawnComponent::HandlePossessedPawnChanged);
	WatchPawn(Controller->GetPawn());
}

void UPDRespawnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	if (AController* Controller = Cast<AController>(GetOwner()))
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(
			this,
			&UPDRespawnComponent::HandlePossessedPawnChanged);
	}
	StopWatchingPawn();

	Super::EndPlay(EndPlayReason);
}

void UPDRespawnComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	WatchPawn(NewPawn);
}

void UPDRespawnComponent::HandleLifeStateChanged(
	EPDLifeState PreviousState,
	EPDLifeState NewState)
{
	const AController* Controller = Cast<AController>(GetOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UWorld* World = GetWorld();
	if (!bRespawnOnDeath || NewState != EPDLifeState::Dead || !Pawn || !World)
	{
		return;
	}

	// 상태 알림은 탈것에서 내린 뒤에 오므로 탄 채로 죽었으면 하차 지점이다.
	PendingSpawnTransform = FTransform(
		FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f),
		Pawn->GetActorLocation());

	// 몸을 이 알림 안에서 바로 지우지 않는다. 0초여도 다음 틱에 다시 시작한다.
	FTimerManager& TimerManager = World->GetTimerManager();
	if (RespawnDelay > 0.0f)
	{
		TimerManager.SetTimer(
			RespawnTimerHandle,
			this,
			&UPDRespawnComponent::HandleRespawnTimer,
			RespawnDelay,
			false);
	}
	else
	{
		RespawnTimerHandle = TimerManager.SetTimerForNextTick(
			this,
			&UPDRespawnComponent::HandleRespawnTimer);
	}
}

void UPDRespawnComponent::HandleRespawnTimer()
{
	RespawnAt(PendingSpawnTransform);
}

void UPDRespawnComponent::WatchPawn(APawn* Pawn)
{
	StopWatchingPawn();

	const APDCharacterBase* Character = Cast<APDCharacterBase>(Pawn);
	UPDHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	if (!Health)
	{
		return;
	}

	Health->OnLifeStateChanged.AddDynamic(this, &UPDRespawnComponent::HandleLifeStateChanged);
	WatchedHealth = Health;
}

void UPDRespawnComponent::StopWatchingPawn()
{
	if (UPDHealthComponent* Health = WatchedHealth.Get())
	{
		Health->OnLifeStateChanged.RemoveDynamic(
			this,
			&UPDRespawnComponent::HandleLifeStateChanged);
	}
	WatchedHealth.Reset();
}
