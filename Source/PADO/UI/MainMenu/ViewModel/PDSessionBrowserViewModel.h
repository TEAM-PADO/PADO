// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PDSessionBrowserViewModel.generated.h"

class UGameInstance;
class UPDSessionEntryViewModel;
class UReusableSteamSessionSubsystem;
struct FReusableSessionEntry;

/**
 * 게임 참가 화면의 공개 방 목록 조회·참가 상태다.
 * 세션 처리는 UPDSteamSessionBlueprintLibrary에 위임하고, 자신이 보낸 요청의 결과만 반영한다.
 */
UCLASS(BlueprintType)
class PADO_API UPDSessionBrowserViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 세션 완료 이벤트에 연결한다. 이미 연결된 상태에서 다시 호출해도 중복 연결하지 않는다. */
	void Initialize(UGameInstance* InGameInstance);

	/** 세션 완료 이벤트 연결을 해제한다. */
	void Deinitialize();

	/** 공개 방 목록을 다시 조회한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Session")
	void RefreshSessions();

	/** 목록에서 선택한 방을 기록한다. nullptr이면 선택을 해제한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Session")
	void SetSelectedEntry(UPDSessionEntryViewModel* InEntry);

	/** 선택한 방에 참가를 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Session")
	void JoinSelectedSession();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "PADO|Session")
	bool CanRefresh() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "PADO|Session")
	bool CanJoin() const;

	const TArray<TObjectPtr<UPDSessionEntryViewModel>>& GetEntries() const { return Entries; }
	bool IsBusy() const { return bIsBusy; }
	const FText& GetStatusText() const { return StatusText; }

private:
	UFUNCTION()
	void HandleFindComplete(bool bSuccess, const TArray<FReusableSessionEntry>& Sessions);

	UFUNCTION()
	void HandleJoinComplete(bool bSuccess, const FString& ConnectStringOrError);

	void SetBusy(bool bInIsBusy);
	void SetStatusText(const FText& InStatusText);
	void SetEntries(const TArray<TObjectPtr<UPDSessionEntryViewModel>>& InEntries);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UPDSessionEntryViewModel>> Entries;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDSessionEntryViewModel> SelectedEntry;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	bool bIsBusy = false;

	/** 조회를 마쳤고 표시할 방이 없을 때 true다. 조회 전에는 false다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	bool bHasNoResults = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	FText StatusText;

	TWeakObjectPtr<UGameInstance> GameInstance;
	TWeakObjectPtr<UReusableSteamSessionSubsystem> SessionSubsystem;
	bool bIsSearchPending = false;
	bool bIsJoinPending = false;
};
