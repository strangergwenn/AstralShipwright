// Astral Shipwright - Gwennaël Arbona

#include "NovaSoundManager.h"

#include "NovaAssetManager.h"
#include "NovaMenuManager.h"

#include "Game/Settings/NovaGameUserSettings.h"
#include "Player/NovaPlayerController.h"
#include "UI/NovaUI.h"
#include "Nova.h"

#include "Components/AudioComponent.h"
#include "AudioDevice.h"

// Statics
UNovaSoundManager* UNovaSoundManager::Singleton = nullptr;

/*----------------------------------------------------
    Audio player instance
----------------------------------------------------*/

FNovaSoundInstance::FNovaSoundInstance(
	UObject* Owner, FNovaSoundInstanceCallback Callback, USoundBase* Sound, bool ChangePitchWithFade, float FadeSpeed)
	: StateCallback(Callback), SoundPitchFade(ChangePitchWithFade), SoundFadeSpeed(FadeSpeed), CurrentVolume(0.0f)
{
	// Create the sound component
	NCHECK(Owner);
	SoundComponent = NewObject<UAudioComponent>(Owner, UAudioComponent::StaticClass());
	NCHECK(SoundComponent);
	SoundComponent->RegisterComponent();

	// Set up the sound component
	SoundComponent->Sound         = Sound;
	SoundComponent->bAutoActivate = false;
	SoundComponent->bAutoDestroy  = false;
}

void FNovaSoundInstance::Update(float DeltaTime)
{
	if (IsValid())
	{
		// Check the state
		bool ShouldPlay = false;
		if (StateCallback.IsBound())
		{
			ShouldPlay = StateCallback.Execute();
		}

		// Determine new volume
		float VolumeDelta = (ShouldPlay ? 1.0f : -1.0f) * DeltaTime;
		float NewVolume   = FMath::Clamp(CurrentVolume + VolumeDelta * SoundFadeSpeed, 0.0f, 1.0f);

		// Update playback
		if (NewVolume != CurrentVolume)
		{
			if (CurrentVolume == 0 && NewVolume != 0)
			{
				SoundComponent->Play();
			}
			else
			{
				SoundComponent->SetVolumeMultiplier(NewVolume);
				if (SoundPitchFade)
				{
					SoundComponent->SetPitchMultiplier(0.5f + 0.5f * NewVolume);
				}
			}
			CurrentVolume = NewVolume;
		}
		else if (CurrentVolume == 0)
		{
			SoundComponent->Stop();
		}
		else if (!SoundComponent->IsPlaying())
		{
			SoundComponent->Play();
		}
	}
}

bool FNovaSoundInstance::IsValid()
{
	return ::IsValid(SoundComponent);
}

bool FNovaSoundInstance::IsIdle()
{
	return !IsValid() || !SoundComponent->IsPlaying();
}

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSoundManager::UNovaSoundManager()
	: Super()

	, PlayerController(nullptr)

	, AudioDevice()
	, CurrentMusicTrack(NAME_None)
	, DesiredMusicTrack(NAME_None)

	, MasterVolume(1.0f)
	, UIVolume(1.0f)
	, EffectsVolume(1.0f)
	, EffectsVolumeMultiplier(1.0f)
	, MusicVolume(1.0f)
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNovaSoundManager::BeginPlay(ANovaPlayerController* PC, FNovaMusicCallback Callback)
{
	NLOG("UNovaSoundManager::BeginPlay");

	// Get references
	PlayerController = PC;
	MusicCallback    = Callback;

	// Get basic game pointers
	const UNovaGameUserSettings* GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
	NCHECK(GameUserSettings);

	// Setup sound settings
	SoundSetup = UNovaAssetManager::Get()->GetDefaultAsset<UNovaSoundSetup>();
	SetMasterVolume(GameUserSettings->MasterVolume);
	SetUIVolume(GameUserSettings->UIVolume);
	SetEffectsVolume(GameUserSettings->EffectsVolume);
	SetMusicVolume(GameUserSettings->MusicVolume);

	// Be safe
	NCHECK(SoundSetup->MasterSoundMix);
	NCHECK(SoundSetup->MasterSoundClass);
	NCHECK(SoundSetup->UISoundClass);
	NCHECK(SoundSetup->EffectsSoundClass);
	NCHECK(SoundSetup->MusicSoundClass);

	// Fetch and map the musical tracks
	MusicCatalog.Empty();
	if (SoundSetup)
	{
		for (const FNovaMusicCatalogEntry& Entry : SoundSetup->Tracks)
		{
			MusicCatalog.Add(Entry.Name, Entry.Tracks);
		}
	}

	// Initialize the sound device and master mix
	AudioDevice = PC->GetWorld()->GetAudioDevice();
	if (AudioDevice)
	{
		AudioDevice->SetBaseSoundMix(SoundSetup->MasterSoundMix);
	}

	// Initialize the music instance
	EnvironmentSoundInstances.Empty();
	MusicSoundInstance = FNovaSoundInstance(PlayerController,    //
		FNovaSoundInstanceCallback::CreateLambda(
			[this]()
			{
				return CurrentMusicTrack == DesiredMusicTrack;
			}),
		nullptr, false, SoundSetup->MusicFadeSpeed);
}

void UNovaSoundManager::Mute()
{
	NLOG("UNovaSoundManager::Mute");

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MasterSoundClass, 0.0f, 1.0f, ENovaUIConstants::FadeDurationShort, true);
	}
}

void UNovaSoundManager::UnMute()
{
	NLOG("UNovaSoundManager::UnMute");

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MasterSoundClass, MasterVolume, 1.0f, ENovaUIConstants::FadeDurationShort, true);
	}
}

void UNovaSoundManager::AddEnvironmentSound(FName SoundName, FNovaSoundInstanceCallback Callback, bool ChangePitchWithFade, float FadeSpeed)
{
	NCHECK(IsValid(PlayerController));

	const FNovaEnvironmentSoundEntry* EnvironmentSound = SoundSetup->Sounds.Find(SoundName);
	if (EnvironmentSound)
	{
		EnvironmentSoundInstances.Add(FNovaSoundInstance(
			PlayerController, Callback, EnvironmentSound->Sound, EnvironmentSound->ChangePitchWithFade, EnvironmentSound->SoundFadeSpeed));
	}
}

void UNovaSoundManager::SetMasterVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetMasterVolume %d", Volume);

	MasterVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MasterSoundClass, MasterVolume, 1.0f, ENovaUIConstants::FadeDurationLong, true);
	}
}

void UNovaSoundManager::SetUIVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetUIVolume %d", Volume);

	UIVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->UISoundClass, UIVolume, 1.0f, ENovaUIConstants::FadeDurationLong, true);
	}
}

void UNovaSoundManager::SetEffectsVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetEffectsVolume %d", Volume);

	EffectsVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	// Do not set the volume since it's done on tick
}

void UNovaSoundManager::SetMusicVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetMusicVolume %d", Volume);

	MusicVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice)
	{
		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->MusicSoundClass, MusicVolume, 1.0f, ENovaUIConstants::FadeDurationLong, true);
	}
}

void UNovaSoundManager::Tick(float DeltaSeconds)
{
	// Control the music track
	if (MusicSoundInstance.IsValid())
	{
		DesiredMusicTrack = MusicCallback.IsBound() ? MusicCallback.Execute() : NAME_None;

		if (CurrentMusicTrack != DesiredMusicTrack || MusicSoundInstance.IsIdle())
		{
			int32 TrackIndex = FMath::RandHelper(MusicCatalog[DesiredMusicTrack].Num());

			NLOG("UNovaSoundManager::Tick : switching track from '%s' to '%s' %d", *CurrentMusicTrack.ToString(),
				*DesiredMusicTrack.ToString(), TrackIndex);

			MusicSoundInstance.SoundComponent->SetSound(MusicCatalog[DesiredMusicTrack][TrackIndex]);
			CurrentMusicTrack = DesiredMusicTrack;
		}

		MusicSoundInstance.Update(DeltaSeconds);
	}

	// Update all sound instances
	for (FNovaSoundInstance& Sound : EnvironmentSoundInstances)
	{
		Sound.Update(DeltaSeconds);
	}

	// Check if we should fade out audio effects
	if (AudioDevice)
	{
		UNovaMenuManager* MenuManager = UNovaMenuManager::Get();

		if (MenuManager->IsMenuOpening() && SoundSetup->FadeEffectsInMenus)
		{
			EffectsVolumeMultiplier -= DeltaSeconds / ENovaUIConstants::FadeDurationShort;
		}
		else
		{
			EffectsVolumeMultiplier += DeltaSeconds / ENovaUIConstants::FadeDurationShort;
		}
		EffectsVolumeMultiplier = FMath::Clamp(EffectsVolumeMultiplier, 0.01f, 1.0f);

		AudioDevice->SetSoundMixClassOverride(
			SoundSetup->MasterSoundMix, SoundSetup->EffectsSoundClass, EffectsVolumeMultiplier * EffectsVolume, 1.0f, 0.0f, true);
	}
}
