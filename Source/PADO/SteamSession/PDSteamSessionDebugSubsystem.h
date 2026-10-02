#pragma once

#include "CoreMinimal.h"
#include "ReusableSessionTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PDSteamSessionDebugSubsystem.generated.h"

class IConsoleObject;
class UReusableSteamSessionSubsystem;

/**
 * 최종 UI 없이 Steam 세션을 검증하기 위한 개발용 콘솔 명령 Subsystem이다.
 * Shipping 빌드에서는 콘솔 명령과 세션 이벤트 바인딩을 등록하지 않는다.
 */
UCLASS()
class PADO_API UPDSteamSessionDebugSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
#if !UE_BUILD_SHIPPING
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();
	void ExecuteCreateCommand(const TArray<FString>& Arguments);
	void ExecuteCreateProtectedCommand(const TArray<FString>& Arguments);
	void ExecuteFindCommand(const TArray<FString>& Arguments);
	void ExecuteJoinCommand(const TArray<FString>& Arguments);
	void ExecuteDestroyCommand(const TArray<FString>& Arguments);
	UReusableSteamSessionSubsystem* GetSessionSubsystem() const;
#endif

	UFUNCTION()
	void HandleCreateComplete(bool bSuccess, const FString& Error);

	UFUNCTION()
	void HandleFindComplete(bool bSuccess, const TArray<FReusableSessionEntry>& Sessions);

	UFUNCTION()
	void HandleJoinComplete(bool bSuccess, const FString& ConnectString);

	UFUNCTION()
	void HandleDestroyComplete(bool bSuccess, const FString& Error);

	TArray<FBlueprintSessionResult> LastFoundSessions;

#if !UE_BUILD_SHIPPING
	IConsoleObject* CreateCommand = nullptr;
	IConsoleObject* CreateProtectedCommand = nullptr;
	IConsoleObject* FindCommand = nullptr;
	IConsoleObject* JoinCommand = nullptr;
	IConsoleObject* DestroyCommand = nullptr;
#endif
};
