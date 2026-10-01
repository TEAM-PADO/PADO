// Copyright PADO. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FindSessionsCallbackProxy.h"
#include "MVVMViewModelBase.h"
#include "PDSessionEntryViewModel.generated.h"

struct FReusableSessionEntry;

/** 게임 참가 목록의 방 한 줄에 표시할 상태다. */
UCLASS(BlueprintType)
class PADO_API UPDSessionEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 세션 검색 결과 한 건으로 표시 상태를 채운다. */
	void InitializeFromSession(const FReusableSessionEntry& InEntry);

	const FText& GetRoomName() const { return RoomName; }
	bool IsFull() const { return bIsFull; }
	const FBlueprintSessionResult& GetSessionResult() const { return SessionResult; }

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	FText RoomName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	FText HostName;

	/** "현재/정원" 형식의 인원 문구다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	FText PlayerCountText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	FText PingText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "PADO|Session", meta = (AllowPrivateAccess = "true"))
	bool bIsFull = false;

	/** 참가 요청에 그대로 넘기는 검색 결과다. 화면에는 표시하지 않는다. */
	FBlueprintSessionResult SessionResult;
};
