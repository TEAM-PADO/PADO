#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ReusableSessionTypes.h"
#include "ReusableSteamSessionSubsystem.generated.h"

/**
 * Project-independent OnlineSubsystem session manager. Steam is selected by DefaultEngine.ini;
 * the same API also works with Null/LAN during local development.
 */
UCLASS()
class REUSABLESTEAMSESSIONS_API UReusableSteamSessionSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Steam Sessions")
	void CreateListenSession(const FReusableSessionSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Steam Sessions")
	void FindSessions(const FString& ProductId, int32 MaxResults = 50, bool bIncludeFriendsOnly = false);

	UFUNCTION(BlueprintCallable, Category = "Steam Sessions")
	void JoinSession(const FBlueprintSessionResult& Session);

	UFUNCTION(BlueprintCallable, Category = "Steam Sessions")
	void DestroySession();

	/** Sends a platform invite. The supplied ID must be a valid ID for the active OSS (Steam64 for Steam). */
	UFUNCTION(BlueprintCallable, Category = "Steam Sessions")
	bool InviteFriend(const FString& FriendUniqueNetId);

	UFUNCTION(BlueprintPure, Category = "Steam Sessions")
	bool IsOperationInProgress() const { return Operation != EOperation::None; }

	UFUNCTION(BlueprintPure, Category = "Steam Sessions")
	bool IsCurrentSessionFull() const;

	UPROPERTY(BlueprintAssignable, Category = "Steam Sessions|Events")
	FReusableSessionOperationComplete OnCreateComplete;
	UPROPERTY(BlueprintAssignable, Category = "Steam Sessions|Events")
	FReusableSessionJoinComplete OnJoinComplete;
	UPROPERTY(BlueprintAssignable, Category = "Steam Sessions|Events")
	FReusableSessionOperationComplete OnDestroyComplete;
	UPROPERTY(BlueprintAssignable, Category = "Steam Sessions|Events")
	FReusableSessionSearchComplete OnFindComplete;
	UPROPERTY(BlueprintAssignable, Category = "Steam Sessions|Events")
	FReusableSessionInviteAccepted OnInviteAccepted;

private:
	enum class EOperation : uint8 { None, Creating, Joining, Destroying, Finding };

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> ActiveSearch;
	FReusableSessionSettings PendingCreateSettings;
	FBlueprintSessionResult PendingJoinResult;
	bool bCreateAfterDestroy = false;
	bool bJoinAfterDestroy = false;
	EOperation Operation = EOperation::None;
	FDelegateHandle CreateHandle, JoinHandle, DestroyHandle, FindHandle, InviteHandle;

	void BeginCreate(const FReusableSessionSettings& Settings);
	void FailCreate(const FString& Error);
	void FailJoin(const FString& Error);
	ULocalPlayer* GetLocalPlayer() const;
	void HandleCreateComplete(FName SessionName, bool bSuccess);
	void HandleJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroyComplete(FName SessionName, bool bSuccess);
	void HandleFindComplete(bool bSuccess);
	void HandleInviteAccepted(bool bSuccess, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& Result);
};
