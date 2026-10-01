// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PDMainMenuViewModel.generated.h"

class UGameInstance;
class UReusableSteamSessionSubsystem;
struct FBlueprintSessionResult;

/**
 * 메인 메뉴 전체에 걸친 상태다. 어느 화면에 있든 받은 Steam 초대의 참가 진행과 실패를 안내한다.
 * 초대 수락 시 참가 요청은 세션 플러그인이 직접 보내므로 이 Viewmodel은 결과만 표시한다.
 */
UCLASS(BlueprintType)
class PADO_API UPDMainMenuViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 초대·참가 완료 이벤트에 연결한다. 이미 연결된 상태에서 다시 호출해도 중복 연결하지 않는다. */
	void Initialize(UGameInstance* InGameInstance);

	/** 초대·참가 완료 이벤트 연결을 해제한다. */
	void Deinitialize();

	/** 표시 중인 안내 문구를 지운다. */
	UFUNCTION(BlueprintCallable, Category = "PADO|MainMenu")
	void ClearNotice();

	bool IsBusy() const { return bIsBusy; }
	const FText& GetNoticeText() const { return NoticeText; }

private:
	UFUNCTION()
	void HandleInviteAccepted(const FBlueprintSessionResult& Session);

	UFUNCTION()
	void HandleJoinComplete(bool bSuccess, const FString& ConnectStringOrError);

	/** 초대 참가 중에는 메뉴 입력을 잠근다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|MainMenu", meta = (AllowPrivateAccess = "true"))
	bool bIsBusy = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|MainMenu", meta = (AllowPrivateAccess = "true"))
	FText NoticeText;

	TWeakObjectPtr<UReusableSteamSessionSubsystem> SessionSubsystem;
	bool bIsInviteJoinPending = false;
};
