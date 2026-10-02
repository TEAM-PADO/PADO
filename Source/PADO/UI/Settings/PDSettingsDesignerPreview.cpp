// Copyright PADO. All Rights Reserved.

#include "PADO/UI/Settings/PDSettingsDesignerPreview.h"

#if WITH_EDITOR
#include "InputCoreTypes.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "PADO/Settings/Struct/PDKeyBindingEntry.h"
#include "PADO/UI/Settings/ViewModel/PDSettingsViewModel.h"
#include "UObject/Package.h"

namespace PDSettingsDesignerPreview
{
	struct FPreviewKeyBinding
	{
		const TCHAR* MappingName;
		const FKey& Key;
	};

	/**
	 * 키 설정 탭 미리보기에 쓸 예시 동작이다. 매핑 이름은 String Table의 Keybind.<이름>과 맞춰
	 * 실제 문구로 보이게 한다. 입력 에셋에 바꿀 수 있는 키가 아직 없어 예시로 채운다.
	 */
	TArray<FPDKeyBindingEntry> MakePreviewKeyBindings()
	{
		const FPreviewKeyBinding PreviewKeyBindings[] =
		{
			{ TEXT("MoveForward"), EKeys::W },
			{ TEXT("Jump"), EKeys::SpaceBar },
			{ TEXT("Sprint"), EKeys::LeftShift },
			{ TEXT("Interact"), EKeys::E },
			{ TEXT("Reload"), EKeys::R },
		};

		TArray<FPDKeyBindingEntry> KeyBindings;
		for (const FPreviewKeyBinding& Preview : PreviewKeyBindings)
		{
			FPDKeyBindingEntry& Entry = KeyBindings.AddDefaulted_GetRef();
			Entry.MappingName = Preview.MappingName;
			Entry.DisplayName = FText::FromName(Entry.MappingName);
			Entry.CurrentKey = Preview.Key;
			Entry.DefaultKey = Preview.Key;
		}

		return KeyBindings;
	}

	UPDSettingsViewModel* CreateViewModel(UObject* Outer)
	{
		// 엔진이 쓰는 설정 객체 대신 기본값을 담은 임시 객체를 쓴다.
		UPDGameUserSettings* PreviewSettings = NewObject<UPDGameUserSettings>(GetTransientPackage());
		PreviewSettings->SetToDefaults();

		UPDSettingsViewModel* ViewModel = NewObject<UPDSettingsViewModel>(Outer);
		ViewModel->InitializeWithKeyBindings(PreviewSettings, MakePreviewKeyBindings());
		return ViewModel;
	}
}
#endif
