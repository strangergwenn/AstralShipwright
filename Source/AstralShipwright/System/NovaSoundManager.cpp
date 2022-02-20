// Astral Shipwright - Gwennaël Arbona

#include "NovaSoundManager.h"

#include "NovaAssetManager.h"
#include "NovaMenuManager.h"
#include "NovaGameInstance.h"

#include "Player/NovaPlayerController.h"
#include "UI/NovaUI.h"
#include "Nova.h"

#include "Components/AudioComponent.h"
#include "AudioDevice.h"

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
	, MusicVolume(1.0f)
	, EffectsVolume(1.0f)
	, EffectsVolumeMultiplier(1.0f)
{}

/*----------------------------------------------------
    Public methods
----------------------------------------------------*/

void UNovaSoundManager::BeginPlay(ANovaPlayerController* PC)
{
	NLOG("UNovaSoundManager::BeginPlay");

	// Get references
	PlayerController                      = PC;
	UNovaGameInstance*       GameInstance = PC->GetGameInstance<UNovaGameInstance>();
	const UNovaAssetManager* AssetManager = GameInstance->GetAssetManager();
	NCHECK(AssetManager);
	const UNovaSoundSetup* SoundSetup = AssetManager->GetDefaultAsset<UNovaSoundSetup>();

	// Fetch the main sound assets
	MasterSoundMix    = SoundSetup->MasterSoundMix;
	MasterSoundClass  = SoundSetup->MasterSoundClass;
	MusicSoundClass   = SoundSetup->MusicSoundClass;
	EffectsSoundClass = SoundSetup->EffectsSoundClass;

	// Fetch and map the musical tracks
	MusicTracks.Empty();
	if (SoundSetup)
	{
		for (const FNovaMusicCatalogEntry& Entry : SoundSetup->Tracks)
		{
			MusicTracks.Add(Entry.Name, Entry.Track);
		}
	}

	// Initialize the sound device and master mix
	AudioDevice = GameInstance->GetWorld()->GetAudioDevice();
	if (AudioDevice)
	{
		NCHECK(MasterSoundMix);
		AudioDevice->SetBaseSoundMix(MasterSoundMix);
	}

	// Initialize the music instance
	SoundInstances.Empty();
	MusicInstance = FNovaSoundInstance(PlayerController,    //
		FNovaSoundInstanceCallback::CreateLambda(
			[this]()
			{
				return CurrentMusicTrack == DesiredMusicTrack;
			}),
		nullptr, false, 0.2f);
}

void UNovaSoundManager::SetMusicTrack(FName Track)
{
	NLOG("UNovaSoundManager::SetMusicTrack : '%s'", *Track.ToString());

	NCHECK(MusicTracks.Contains(Track));

	DesiredMusicTrack = Track;
}

void UNovaSoundManager::AddSound(USoundBase* Sound, FNovaSoundInstanceCallback Callback, bool ChangePitchWithFade, float FadeSpeed)
{
	NCHECK(IsValid(PlayerController));
	SoundInstances.Add(FNovaSoundInstance(PlayerController, Callback, Sound, ChangePitchWithFade, FadeSpeed));
}

void UNovaSoundManager::SetMasterVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetMasterVolume %d", Volume);

	MasterVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice && MasterSoundMix && MasterSoundClass)
	{
		AudioDevice->SetSoundMixClassOverride(
			MasterSoundMix, MasterSoundClass, MasterVolume, 1.0f, ENovaUIConstants::FadeDurationLong, true);
	}
}

void UNovaSoundManager::SetMusicVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetMusicVolume %d", Volume);

	MusicVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);

	if (AudioDevice && MasterSoundMix && MusicSoundClass)
	{
		AudioDevice->SetSoundMixClassOverride(MasterSoundMix, MusicSoundClass, MusicVolume, 1.0f, ENovaUIConstants::FadeDurationLong, true);
	}
}

void UNovaSoundManager::SetEffectsVolume(int32 Volume)
{
	NLOG("UNovaSoundManager::SetEffectsVolume %d", Volume);

	EffectsVolume = FMath::Clamp(Volume / 10.0f, 0.0f, 1.0f);
}

void UNovaSoundManager::Tick(float DeltaSeconds)
{
	// Control the music track
	if (MusicInstance.IsValid())
	{
		if (CurrentMusicTrack != DesiredMusicTrack && MusicInstance.IsIdle())
		{
			NLOG("UNovaSoundManager::Tick : switching track from '%s' to '%s'", *CurrentMusicTrack.ToString(),
				*DesiredMusicTrack.ToString());

			MusicInstance.SoundComponent->SetSound(MusicTracks[DesiredMusicTrack]);
			CurrentMusicTrack = DesiredMusicTrack;
		}

		MusicInstance.Update(DeltaSeconds);
	}

	// Update all sound instances
	for (FNovaSoundInstance& Sound : SoundInstances)
	{
		Sound.Update(DeltaSeconds);
	}

	// Check if we should fade out audio effects
	if (AudioDevice && MasterSoundMix && EffectsSoundClass)
	{
		UNovaMenuManager* MenuManager = UNovaMenuManager::Get();

		if (MenuManager->IsMenuOpening())
		{
			EffectsVolumeMultiplier -= DeltaSeconds / ENovaUIConstants::FadeDurationShort;
		}
		else
		{
			EffectsVolumeMultiplier += DeltaSeconds / ENovaUIConstants::FadeDurationShort;
		}
		EffectsVolumeMultiplier = FMath::Clamp(EffectsVolumeMultiplier, 0.01f, 1.0f);

		AudioDevice->SetSoundMixClassOverride(MasterSoundMix, EffectsSoundClass, EffectsVolumeMultiplier * EffectsVolume, 1.0f, 0.0f, true);
	}
}
