#include "PADO/Core/PDGameState.h"

#include "Net/UnrealNetwork.h"
#include "PADO/PADO.h"

FPDActiveDeliveryState APDGameState::GetActiveDeliveryState() const
{
	return ActiveDeliveryState;
}

int32 APDGameState::GetCompletedGeneralDeliveryCount() const
{
	return CompletedGeneralDeliveryCount;
}

void APDGameState::SetActiveDeliveryState(const FPDActiveDeliveryState& NewActiveDeliveryState)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPDServer, Warning, TEXT("SetActiveDeliveryState was ignored because only the server can change shared delivery state."));
		return;
	}

	ActiveDeliveryState = NewActiveDeliveryState;
	OnRep_ActiveDeliveryState();
}

void APDGameState::ClearActiveDeliveryState()
{
	SetActiveDeliveryState(FPDActiveDeliveryState());
}

void APDGameState::SetCompletedGeneralDeliveryCount(const int32 NewCompletedGeneralDeliveryCount)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPDServer, Warning, TEXT("SetCompletedGeneralDeliveryCount was ignored because only the server can change shared delivery state."));
		return;
	}

	const int32 PreviousCompletedGeneralDeliveryCount = CompletedGeneralDeliveryCount;
	CompletedGeneralDeliveryCount = FMath::Max(0, NewCompletedGeneralDeliveryCount);
	OnRep_CompletedGeneralDeliveryCount(PreviousCompletedGeneralDeliveryCount);
}

void APDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APDGameState, ActiveDeliveryState);
	DOREPLIFETIME(APDGameState, CompletedGeneralDeliveryCount);
}

void APDGameState::OnRep_ActiveDeliveryState()
{
	OnActiveDeliveryStateChanged.Broadcast(ActiveDeliveryState);
}

void APDGameState::OnRep_CompletedGeneralDeliveryCount(const int32 PreviousCompletedGeneralDeliveryCount)
{
	if (PreviousCompletedGeneralDeliveryCount != CompletedGeneralDeliveryCount)
	{
		OnCompletedGeneralDeliveryCountChanged.Broadcast(CompletedGeneralDeliveryCount);
	}
}
