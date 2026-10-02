// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PADO/SteamSession/Enum/PDRoomSessionFlowState.h"
#include "ReusableSessionTypes.h"
#include "PDRoomSessionFlowSubsystem.generated.h"

class APDGameMode;
class UReusableSteamSessionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPDRoomSessionFlowStateChangedSignature, EPDRoomSessionFlowState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDRoomSessionCreateCompletedSignature, bool, bSuccess, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDRoomSessionFindCompletedSignature, bool, bSuccess, const TArray<FReusableSessionEntry>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDRoomSessionJoinCompletedSignature, bool, bSuccess, const FString&, ConnectStringOrError);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPDRoomSessionExitCompletedSignature, bool, bSuccess, bool, bWasHostExit, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPDRoomSessionInviteAcceptedSignature);

/**
 * 메뉴와 인게임 사이의 PADO 방 흐름을 조율하는 GameInstance 서비스입니다.
 * Steam 세션의 생성·검색·참가·종료와 PADO 저장·맵 이동의 순서를 한곳에서 관리합니다.
 * 이후 Asset Manager 프리로드와 이어하기 복원 단계를 같은 상태 기계에 추가합니다.
 */
UCLASS()
class PADO_API UPDRoomSessionFlowSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * 새 방의 저장 컨텍스트를 만들고 PADO Steam 리슨 세션 생성을 시작합니다.
	 * 완료 후 호스트를 설정된 인게임 맵으로 이동시킵니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PADO|Room Session Flow")
	bool RequestCreateRoom(const FString& RoomName);

	/** PADO ProductId를 사용하는 공개 Steam 방 목록 검색을 시작합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Room Session Flow", meta = (ClampMin = "1", ClampMax = "100"))
	bool RequestFindRooms(int32 MaxResults = 50);

	/** 선택한 Steam 검색 결과 참가를 시작하고 성공 시 호스트가 제공한 연결 문자열로 이동합니다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|Room Session Flow")
	bool RequestJoinRoom(const FBlueprintSessionResult& Session);

	/**
	 * 현재 방에서 나가고 메인 메뉴로 돌아가도록 요청합니다.
	 * 호스트는 저장 성공 뒤 세션을 삭제하고 전원을 메인 메뉴로 이동시키며, 클라이언트는 본인만 나갑니다.
	 *
	 * @return 요청을 시작했으면 true입니다. 최종 결과는 OnRoomExitCompleted에서 확인합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "PADO|Room Session Flow")
	bool RequestReturnToMainMenu();

	/** @return 현재 진행 중인 방 흐름 단계입니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Room Session Flow")
	EPDRoomSessionFlowState GetFlowState() const { return FlowState; }

	/** @return 저장 또는 세션 종료·메뉴 복귀 작업이 진행 중이면 true입니다. */
	UFUNCTION(BlueprintPure, Category = "PADO|Room Session Flow")
	bool IsFlowOperationInProgress() const { return FlowState != EPDRoomSessionFlowState::Idle; }

	/** 방 흐름 단계가 바뀔 때 발생합니다. UI는 이 이벤트로 입력을 잠글 수 있습니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Room Session Flow|Events")
	FPDRoomSessionFlowStateChangedSignature OnFlowStateChanged;

	/** 새 방 생성 흐름의 최종 결과입니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Room Session Flow|Events")
	FPDRoomSessionCreateCompletedSignature OnRoomCreateCompleted;

	/** 공개 방 검색 흐름의 최종 결과입니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Room Session Flow|Events")
	FPDRoomSessionFindCompletedSignature OnRoomFindCompleted;

	/** 검색 또는 Steam 초대를 통한 참가 흐름의 최종 결과입니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Room Session Flow|Events")
	FPDRoomSessionJoinCompletedSignature OnRoomJoinCompleted;

	/** Steam 초대를 수락해 Flow가 참가 절차를 시작했을 때 발생합니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Room Session Flow|Events")
	FPDRoomSessionInviteAcceptedSignature OnRoomInviteAccepted;

	/** 나가기 또는 호스트 종료 흐름의 최종 결과입니다. UI는 결과만 표시하고 순서를 직접 처리하지 않습니다. */
	UPROPERTY(BlueprintAssignable, Category = "PADO|Room Session Flow|Events")
	FPDRoomSessionExitCompletedSignature OnRoomExitCompleted;

private:
	UReusableSteamSessionSubsystem* GetSteamSessionSubsystem() const;
	APDGameMode* GetHostingGameMode() const;
	FString GetListenServerMapPackageName() const;
	FString GetMainMenuMapPackageName() const;
	bool StartSteamSessionCreation(const FString& RoomName);
	void CompleteCreate(bool bSuccess, const FString& Error);
	void CompleteFind(bool bSuccess, const TArray<FReusableSessionEntry>& Sessions);
	void CompleteJoin(bool bSuccess, const FString& ConnectStringOrError);
	void BeginSessionDestruction();
	void ReturnToMainMenu();
	void CompleteExit(bool bSuccess, const FString& Error);
	void SetFlowState(EPDRoomSessionFlowState NewState);

	UFUNCTION()
	void HandleRoomSaveCompleted(bool bSuccess, const FString& RoomSaveId, const FString& Error);

	UFUNCTION()
	void HandleSessionCreateCompleted(bool bSuccess, const FString& Error);

	UFUNCTION()
	void HandleSessionFindCompleted(bool bSuccess, const TArray<FReusableSessionEntry>& Sessions);

	UFUNCTION()
	void HandleSessionJoinCompleted(bool bSuccess, const FString& ConnectStringOrError);

	UFUNCTION()
	void HandleSteamInviteAccepted(const FBlueprintSessionResult& Session);

	UFUNCTION()
	void HandleSessionDestroyCompleted(bool bSuccess, const FString& Error);

	/** 현재 요청이 호스트 종료인지 여부입니다. */
	bool bHostExitRequest = false;

	/** 생성 성공 뒤 호스트가 이동할 인게임 맵의 긴 패키지 경로입니다. */
	FString PendingListenServerMap;

	/** UI에 복제하지 않는 로컬 비동기 흐름 상태입니다. */
	EPDRoomSessionFlowState FlowState = EPDRoomSessionFlowState::Idle;
};
