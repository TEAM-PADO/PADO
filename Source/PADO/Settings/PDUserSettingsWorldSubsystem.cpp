// Copyright PADO. All Rights Reserved.

#include "PADO/Settings/PDUserSettingsWorldSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PADO/PADO.h"
#include "PADO/Settings/PDAudioSettings.h"
#include "PADO/Settings/PDGameUserSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace PDUserSettingsWorldSubsystem
{
	/** 표시 설정을 걸어 둔 PIE 월드 수다. 0이 되면 에디터 화면을 되돌린다. */
	int32 EditorPresentationWorldCount = 0;
}

bool UPDUserSettingsWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 전용 서버에는 소리를 내거나 화면을 그릴 장치가 없다.
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UPDUserSettingsWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UPDGameUserSettings* Settings = UPDGameUserSettings::Get();
	if (Settings)
	{
		VolumesChangedHandle = Settings->OnVolumesChanged.AddUObject(this, &ThisClass::ApplyVolumes);
	}

#if WITH_EDITOR
	// 패키징 빌드는 엔진 시작 때 이미 적용한다. 에디터는 PIE마다 걸고 끝날 때 되돌린다.
	if (GIsEditor && Settings && InWorld.WorldType == EWorldType::PIE)
	{
		Settings->ApplyPresentationSettings();
		bAppliedEditorPresentation = true;
		++PDUserSettingsWorldSubsystem::EditorPresentationWorldCount;
	}
#endif

	ApplyVolumes();
}

void UPDUserSettingsWorldSubsystem::Deinitialize()
{
	UPDGameUserSettings* Settings = UPDGameUserSettings::Get();
	if (Settings)
	{
		Settings->OnVolumesChanged.Remove(VolumesChangedHandle);
	}
	VolumesChangedHandle.Reset();

	if (PushedSoundMix)
	{
		UGameplayStatics::PopSoundMixModifier(GetWorld(), PushedSoundMix);
		PushedSoundMix = nullptr;
	}

#if WITH_EDITOR
	if (bAppliedEditorPresentation)
	{
		bAppliedEditorPresentation = false;
		if (--PDUserSettingsWorldSubsystem::EditorPresentationWorldCount <= 0)
		{
			PDUserSettingsWorldSubsystem::EditorPresentationWorldCount = 0;
			if (Settings)
			{
				Settings->RestoreEditorPresentation();
			}
		}
	}
#endif

	Super::Deinitialize();
}

void UPDUserSettingsWorldSubsystem::ApplyVolumes()
{
	const UPDGameUserSettings* Settings = UPDGameUserSettings::Get();
	if (!Settings)
	{
		UE_LOG(LogPADO, Warning, TEXT("Volume settings were not applied. Set GameUserSettingsClassName to PDGameUserSettings."));
		return;
	}

	const UPDAudioSettings* AudioSettings = GetDefault<UPDAudioSettings>();
	USoundMix* SoundMix = AudioSettings->UserVolumeSoundMix.LoadSynchronous();
	if (!SoundMix)
	{
		UE_LOG(LogPADO, Warning, TEXT("Volume settings were not applied. Assign User Volume Sound Mix in Project Settings > PADO Audio."));
		return;
	}

	for (const TPair<EPDVolumeCategory, TSoftObjectPtr<USoundClass>>& Pair : AudioSettings->VolumeSoundClasses)
	{
		USoundClass* SoundClass = Pair.Value.LoadSynchronous();
		if (!SoundClass)
		{
			UE_LOG(LogPADO, Warning, TEXT("Volume category %s has no sound class in Project Settings > PADO Audio."), *UEnum::GetValueAsString(Pair.Key));
			continue;
		}

		// 페이드 없이 바로 바꾸고, 하위 Sound Class에도 곱해지게 한다.
		UGameplayStatics::SetSoundMixClassOverride(GetWorld(), SoundMix, SoundClass, Settings->GetEffectiveVolume(Pair.Key), 1.0f, 0.0f, true);
	}

	if (!PushedSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(GetWorld(), SoundMix);
		PushedSoundMix = SoundMix;
	}
}

bool UPDUserSettingsWorldSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}
