// Copyright PADO. All Rights Reserved.

#include "PADO/Settings/PDKeyBindingSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameplayTagContainer.h"
#include "InputMappingContext.h"
#include "PADO/PADO.h"
#include "PADO/Settings/PDControlsSettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace PDKeyBindingSubsystem
{
	/** 사용자 설정에 쓸 키 변경 하나다. */
	struct FPendingKeyChange
	{
		FName MappingName;
		EPlayerMappableKeySlot Slot;
		FName HardwareDeviceId;
		FKey NewKey;
	};
}

void UPDKeyBindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 사용자 설정은 Enhanced Input 로컬 플레이어 서브시스템이 초기화하면서 만든다.
	Collection.InitializeDependency<UEnhancedInputLocalPlayerSubsystem>();
	Super::Initialize(Collection);

	RegisterMappingContexts();
}

bool UPDKeyBindingSubsystem::IsAvailable() const
{
	return GetUserSettings() != nullptr;
}

TArray<FPDKeyBindingEntry> UPDKeyBindingSubsystem::GetKeyBindings() const
{
	TArray<FPDKeyBindingEntry> Entries;
	const UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	if (!Profile)
	{
		return Entries;
	}

	RegisterMappingContexts();
	for (const FName MappingName : GetOrderedMappingNames())
	{
		const FKeyMappingRow* Row = Profile->FindKeyMappingRow(MappingName);
		const FPlayerKeyMapping* Mapping = Row ? FindKeyboardMapping(*Row) : nullptr;
		if (!Mapping)
		{
			continue;
		}

		FPDKeyBindingEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.MappingName = MappingName;
		Entry.DisplayName = Mapping->GetDisplayName();
		Entry.CurrentKey = Mapping->GetCurrentKey();
		Entry.DefaultKey = Mapping->GetDefaultKey();
	}

	return Entries;
}

void UPDKeyBindingSubsystem::ApplyKeyBindings(const TMap<FName, FKey>& Keys)
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	if (!Profile)
	{
		UE_LOG(LogPADO, Warning, TEXT("Key bindings were not applied. Enable User Settings in Project Settings > Enhanced Input."));
		return;
	}

	// 사용자 설정을 바꾸면 행이 다시 만들어지므로 바꿀 내용을 먼저 모은다.
	TArray<PDKeyBindingSubsystem::FPendingKeyChange> Changes;
	for (const TPair<FName, FKey>& Pair : Keys)
	{
		const FKeyMappingRow* Row = Profile->FindKeyMappingRow(Pair.Key);
		const FPlayerKeyMapping* PrimaryMapping = Row ? FindKeyboardMapping(*Row) : nullptr;
		if (!PrimaryMapping)
		{
			continue;
		}

		// 견착·조준처럼 한 기본 키를 나눠 쓰는 매핑은 함께 바꿔야 같은 키로 계속 동작한다.
		for (const FPlayerKeyMapping& Mapping : Row->Mappings)
		{
			if (Mapping.GetDefaultKey() != PrimaryMapping->GetDefaultKey() || Mapping.GetCurrentKey() == Pair.Value)
			{
				continue;
			}

			Changes.Add({ Pair.Key, Mapping.GetSlot(), Mapping.GetHardwareDeviceId().HardwareDeviceIdentifier, Pair.Value });
		}
	}

	if (Changes.IsEmpty())
	{
		return;
	}

	for (const PDKeyBindingSubsystem::FPendingKeyChange& Change : Changes)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = Change.MappingName;
		Args.Slot = Change.Slot;
		Args.HardwareDeviceId = Change.HardwareDeviceId;
		Args.NewKey = Change.NewKey;

		FGameplayTagContainer FailureReason;
		UserSettings->MapPlayerKey(Args, FailureReason);
		if (!FailureReason.IsEmpty())
		{
			UE_LOG(LogPADO, Warning, TEXT("Could not map %s to %s: %s"), *Change.MappingName.ToString(), *Change.NewKey.ToString(), *FailureReason.ToStringSimple());
		}
	}

	UserSettings->ApplySettings();
	UserSettings->SaveSettings();
}

UEnhancedInputUserSettings* UPDKeyBindingSubsystem::GetUserSettings() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return InputSubsystem ? InputSubsystem->GetUserSettings() : nullptr;
}

void UPDKeyBindingSubsystem::RegisterMappingContexts() const
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	for (const TSoftObjectPtr<UInputMappingContext>& MappingContext : GetDefault<UPDControlsSettings>()->KeyBindingMappingContexts)
	{
		if (const UInputMappingContext* LoadedContext = MappingContext.LoadSynchronous())
		{
			UserSettings->RegisterInputMappingContext(LoadedContext);
		}
	}
}

TArray<FName> UPDKeyBindingSubsystem::GetOrderedMappingNames() const
{
	TArray<FName> MappingNames;
	for (const TSoftObjectPtr<UInputMappingContext>& MappingContext : GetDefault<UPDControlsSettings>()->KeyBindingMappingContexts)
	{
		const UInputMappingContext* LoadedContext = MappingContext.LoadSynchronous();
		if (!LoadedContext)
		{
			continue;
		}

		for (const FEnhancedActionKeyMapping& Mapping : LoadedContext->GetMappings())
		{
			if (Mapping.IsPlayerMappable() && IsKeyboardOrMouseButton(Mapping.Key))
			{
				MappingNames.AddUnique(Mapping.GetMappingName());
			}
		}
	}

	return MappingNames;
}

const FPlayerKeyMapping* UPDKeyBindingSubsystem::FindKeyboardMapping(const FKeyMappingRow& Row)
{
	const FPlayerKeyMapping* FoundMapping = nullptr;
	for (const FPlayerKeyMapping& Mapping : Row.Mappings)
	{
		if (IsKeyboardOrMouseButton(Mapping.GetDefaultKey()) && (!FoundMapping || Mapping.GetSlot() < FoundMapping->GetSlot()))
		{
			FoundMapping = &Mapping;
		}
	}

	return FoundMapping;
}

bool UPDKeyBindingSubsystem::IsKeyboardOrMouseButton(const FKey& Key)
{
	return Key.IsValid() && !Key.IsGamepadKey() && !Key.IsTouch() && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D();
}
