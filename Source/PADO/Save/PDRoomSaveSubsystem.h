// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PADO/Save/Struct/PDRoomPersistentState.h"
#include "PADO/Save/Struct/PDRoomSaveListEntry.h"
#include "PDRoomSaveSubsystem.generated.h"

class UPDRoomSaveGame;
class UPDRoomSaveIndexSaveGame;
class USaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDRoomSaveCompletedSignature, bool, bSuccess, const FString&, RoomSaveId, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FPDRoomLoadCompletedSignature, bool, bSuccess, const FString&, RoomSaveId, UPDRoomSaveGame*, LoadedSaveGame, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDRoomDeleteCompletedSignature, bool, bSuccess, const FString&, RoomSaveId, const FString&, Error);

/**
 * 호스트 로컬의 방 저장본을 관리하는 GameInstance 서비스입니다.
 * 게임 규칙이나 이벤트별 저장 시점은 소유하지 않으며, 서버가 전달한 값 스냅샷만 기록합니다.
 */
UCLASS()
class PADO_API UPDRoomSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 새 게임용 영구 방 ID를 만들고 현재 호스트 저장 컨텍스트로 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	bool BeginNewRoom(const FString& RoomDisplayName, FString& OutRoomSaveId);

	/** 선택한 저장본을 비동기로 불러오고, 성공 시 현재 방 저장 컨텍스트로 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	bool LoadRoomForResume(const FString& RoomSaveId);

	/** 현재 방의 불변 스냅샷을 비동기로 저장합니다. 저장 중인 경우 마지막 요청 하나를 대기시킵니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	bool SaveActiveRoom(const FPDRoomPersistentState& PersistentState);

	/** 현재 로컬 Steam 사용자가 소유한 저장 방 목록을 다시 읽습니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	bool RefreshRoomSaveListEntries();

	/** 현재 로컬 Steam 사용자가 소유한 저장 방 목록 항목을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Save")
	TArray<FPDRoomSaveListEntry> GetRoomSaveListEntries() const;

	/** 현재 로컬 Steam 사용자가 소유한 저장본 하나를 디스크와 목록에서 함께 삭제합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	bool DeleteRoomSave(const FString& RoomSaveId);

	/** 현재 선택된 방 저장 ID를 반환합니다. 새 방 또는 이어하기를 아직 선택하지 않았다면 비어 있습니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Save")
	FString GetActiveRoomSaveId() const;

	/** 현재 선택된 저장본을 반환합니다. 새 방은 첫 저장이 끝나기 전까지 nullptr일 수 있습니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Save")
	UPDRoomSaveGame* GetActiveRoomSaveGame() const;

	/** 저장·불러오기·삭제 작업이 진행 중이면 true입니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Save")
	bool IsOperationInProgress() const;

	/** 세션 종료 뒤 현재 방 컨텍스트를 비웁니다. 디스크 저장본은 유지합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Save")
	void ClearActiveRoomContext();

	/** 비동기 저장이 끝난 결과입니다. 호스트 종료는 성공일 때만 다음 단계로 진행해야 합니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Save|Events")
	FPDRoomSaveCompletedSignature OnRoomSaveCompleted;

	/** 이어하기 대상 저장본을 불러온 결과입니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Save|Events")
	FPDRoomLoadCompletedSignature OnRoomLoadCompleted;

	/** 저장본 삭제 결과입니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Save|Events")
	FPDRoomDeleteCompletedSignature OnRoomDeleteCompleted;

private:
	enum class EOperation : uint8
	{
		None,
		LoadingRoom,
		SavingRoom,
		SavingIndex
	};

	static constexpr int32 SaveUserIndex = 0;

	bool ResolveLocalPlatformUserId(FString& OutPlatformUserId) const;
	bool ValidateRoomSaveId(const FString& RoomSaveId, FString& OutError) const;
	FString GetRoomSaveSlotName(const FString& RoomSaveId) const;
	FString GetIndexSaveSlotName() const;
	UPDRoomSaveIndexSaveGame* LoadIndexSaveGame(FString& OutError) const;
	bool StartSaveActiveRoom(const FPDRoomPersistentState& PersistentState);
	void StartQueuedSaveIfNeeded();
	void UpdateCachedListEntries(const UPDRoomSaveIndexSaveGame& IndexSaveGame, const FString& PlatformUserId);
	void HandleRoomLoadCompleted(const FString& SlotName, int32 UserIndex, USaveGame* LoadedGameData);
	void HandleRoomSaveCompleted(const FString& SlotName, int32 UserIndex, bool bSuccess);
	void HandleIndexSaveCompleted(const FString& SlotName, int32 UserIndex, bool bSuccess);

	/** 현재 진행 중인 비동기 작업입니다. */
	EOperation Operation = EOperation::None;

	/** 새 게임 또는 이어하기로 선택된 현재 방의 영구 ID입니다. */
	FString ActiveRoomSaveId;

	/** 현재 방을 소유한 호스트 Steam 플랫폼 ID 문자열입니다. */
	FString ActiveHostPlatformUserId;

	/** 저장 목록에 보일 현재 방의 이름입니다. */
	FString ActiveRoomDisplayName;

	/** LoadRoomForResume 요청 중인 방 저장 ID입니다. */
	FString PendingLoadRoomSaveId;

	/** SaveGame 비동기 작업 중 GC로부터 보호할 저장본입니다. */
	UPROPERTY(Transient)
	TObjectPtr<UPDRoomSaveGame> ActiveRoomSaveGame;

	/** 현재 방 파일 기록이 끝날 때까지 보관하는 스냅샷 객체입니다. */
	UPROPERTY(Transient)
	TObjectPtr<UPDRoomSaveGame> PendingRoomSaveGame;

	/** 방 파일 기록 뒤 목록 갱신을 기록할 때까지 보관하는 인덱스 객체입니다. */
	UPROPERTY(Transient)
	TObjectPtr<UPDRoomSaveIndexSaveGame> PendingIndexSaveGame;

	/** 목록 저장 완료 콜백에서 결과를 전달할 방 ID입니다. */
	FString PendingIndexRoomSaveId;

	/** 저장 도중 새 저장 요청이 오면 최신 상태 하나만 보관합니다. */
	TOptional<FPDRoomPersistentState> QueuedPersistentState;

	/** 현재 호스트의 저장 목록 카드 캐시입니다. */
	UPROPERTY(Transient)
	TArray<FPDRoomSaveListEntry> CachedRoomSaveListEntries;
};
