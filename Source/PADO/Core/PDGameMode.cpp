#include "PADO/Core/PDGameMode.h"

#include "PADO/Core/PDGameState.h"
#include "PADO/Delivery/Definition/PDDeliveryDefinitionSet.h"
#include "PADO/PADO.h"
#include "PADO/Save/PDRoomSaveSubsystem.h"
#include "PADO/SteamSession/PDRoomSessionFlowSubsystem.h"

#include "PADO/Character/PDPlayerState.h"
#include "Kismet/GameplayStatics.h"

APDGameMode::APDGameMode()
{
	// 플레이어의 Ability System은 PlayerState가 소유한다. 다른 클래스를 쓰면
	// 플레이어 캐릭터가 연결할 ASC가 없다.
	PlayerStateClass = APDPlayerState::StaticClass();
	GameStateClass = APDGameState::StaticClass();
}

UPDDeliveryDefinitionSet* APDGameMode::GetDeliveryDefinitionSet() const
{
	return DeliveryDefinitionSet;
}

bool APDGameMode::CaptureRoomPersistentState(FPDRoomPersistentState& OutPersistentState, FString& OutError) const
{
	OutError.Reset();
	OutPersistentState = FPDRoomPersistentState();

	if (!HasAuthority())
	{
		OutError = TEXT("Only the server can capture room persistent state.");
		return false;
	}

	const APDGameState* PADOGameState = GetGameState<APDGameState>();
	if (!PADOGameState)
	{
		OutError = TEXT("PADO GameState is unavailable.");
		return false;
	}

	OutPersistentState.CompletedGeneralDeliveryCount = PADOGameState->GetCompletedGeneralDeliveryCount();
	return OutPersistentState.Validate(OutError);
}

bool APDGameMode::RestoreRoomPersistentState(const FPDRoomPersistentState& PersistentState, FString& OutError)
{
	OutError.Reset();

	if (!HasAuthority())
	{
		OutError = TEXT("Only the server can restore room persistent state.");
		return false;
	}

	if (!PersistentState.Validate(OutError))
	{
		return false;
	}

	APDGameState* PADOGameState = GetGameState<APDGameState>();
	if (!PADOGameState)
	{
		OutError = TEXT("PADO GameState is unavailable.");
		return false;
	}

	PADOGameState->SetCompletedGeneralDeliveryCount(PersistentState.CompletedGeneralDeliveryCount);
	PADOGameState->ClearActiveDeliveryState();
	return true;
}

bool APDGameMode::RequestRoomSave()
{
	if (!HasAuthority())
	{
		UE_LOG(LogPDServer, Warning, TEXT("RequestRoomSave was ignored because only the server can request a room save."));
		return false;
	}

	FPDRoomPersistentState PersistentState;
	FString CaptureError;
	if (!CaptureRoomPersistentState(PersistentState, CaptureError))
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot request a room save because persistent state capture failed: %s"), *CaptureError);
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UPDRoomSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UPDRoomSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogPDServer, Error, TEXT("Cannot request a room save because the room save subsystem is unavailable."));
		return false;
	}

	return SaveSubsystem->SaveActiveRoom(PersistentState);
}

void APDGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UPDRoomSessionFlowSubsystem* FlowSubsystem = GameInstance ? GameInstance->GetSubsystem<UPDRoomSessionFlowSubsystem>() : nullptr;
	if (!FlowSubsystem || FlowSubsystem->GetActiveRoomAccessPolicy() == EPDRoomAccessPolicy::Public)
	{
		return;
	}

	const FString EncodedPassword = UGameplayStatics::ParseOption(Options, PDRoomAccessOptions::PasswordKey);
	if (FlowSubsystem->IsEncodedRoomAccessPasswordValid(EncodedPassword))
	{
		UE_LOG(LogPDServer, Log, TEXT("Room access accepted with a valid password."));
		return;
	}

	ErrorMessage = TEXT("PADO_ROOM_ACCESS_DENIED");
	UE_LOG(LogPDServer, Warning, TEXT("Room access was denied before player controller and player state creation."));
}

void APDGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!DeliveryDefinitionSet)
	{
		UE_LOG(LogPDServer, Error, TEXT("DeliveryDefinitionSet is not configured. Delivery candidates and acceptance cannot be processed."));
		return;
	}

	FString ValidationError;
	if (!DeliveryDefinitionSet->Validate(ValidationError))
	{
		UE_LOG(LogPDServer, Error, TEXT("DeliveryDefinitionSet '%s' validation failed: %s"), *DeliveryDefinitionSet->GetName(), *ValidationError);
	}
}
