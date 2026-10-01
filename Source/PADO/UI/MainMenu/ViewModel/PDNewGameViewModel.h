// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PDNewGameViewModel.generated.h"

class UGameInstance;
class UReusableSteamSessionSubsystem;

/**
 * 새 게임(방 생성) 화면의 입력과 생성 요청 상태다.
 * 방 이름과 공개/친구(초대 전용)를 입력받아 PADO 리슨 세션 생성을 요청한다.
 */
UCLASS(BlueprintType)
class PADO_API UPDNewGameViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 방 이름 최대 글자 수다. Steam 세션 설정값에 넣기 전 UI에서 제한한다. */
	static constexpr int32 MaxRoomNameLength = 24;

	/**
	 * 친구(초대 전용) 방 생성 지원 여부다.
	 * 현재 UPDSteamSessionBlueprintLibrary::CreatePADOListenSession이 공개 방만 만들므로 false다.
	 * 네트워크 코어가 친구 전용 인자를 추가하면 true로 바꾸고 CreateGame에서 전달한다.
	 */
	static constexpr bool bFriendsOnlySessionSupported = false;

	/** 세션 생성 완료 이벤트에 연결한다. 이미 연결된 상태에서 다시 호출해도 중복 연결하지 않는다. */
	void Initialize(UGameInstance* InGameInstance);

	/** 세션 생성 완료 이벤트 연결을 해제한다. */
	void Deinitialize();

	UFUNCTION(BlueprintCallable, Category = "PADO|NewGame")
	void SetRoomName(const FText& InRoomName);

	/** 친구(초대 전용) 방 여부를 고른다. 지원하지 않으면 공개로 유지한다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|NewGame")
	void SetIsFriendsOnly(bool bInIsFriendsOnly);

	/** 입력한 설정으로 방 생성을 요청한다. 성공하면 플러그인이 게임 지도로 이동시킨다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|NewGame")
	void CreateGame();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "PADO|NewGame")
	bool CanCreate() const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "PADO|NewGame")
	bool CanSelectFriendsOnly() const;

	bool IsFriendsOnly() const { return bIsFriendsOnly; }
	bool IsBusy() const { return bIsBusy; }
	const FText& GetStatusText() const { return StatusText; }

private:
	UFUNCTION()
	void HandleCreateComplete(bool bSuccess, const FString& Error);

	FString GetTrimmedRoomName() const;
	bool IsRoomNameValid() const;
	void SetBusy(bool bInIsBusy);
	void SetStatusText(const FText& InStatusText);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|NewGame", meta = (AllowPrivateAccess = "true"))
	FText RoomName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|NewGame", meta = (AllowPrivateAccess = "true"))
	bool bIsFriendsOnly = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|NewGame", meta = (AllowPrivateAccess = "true"))
	bool bIsBusy = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|NewGame", meta = (AllowPrivateAccess = "true"))
	FText StatusText;

	TWeakObjectPtr<UGameInstance> GameInstance;
	TWeakObjectPtr<UReusableSteamSessionSubsystem> SessionSubsystem;
	bool bIsCreatePending = false;
};
