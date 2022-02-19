// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "NovaAssetManager.h"
#include "NovaSoundManager.generated.h"

/*----------------------------------------------------
    Music catalog
----------------------------------------------------*/

// Sound track entry
USTRUCT()
struct FNovaMusicCatalogEntry
{
	GENERATED_BODY()

	FNovaMusicCatalogEntry() : Name(NAME_None), Track(nullptr)
	{}

	/** Track name */
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	FName Name;

	/** Sound asset */
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	class USoundBase* Track;
};

// Music catalog
UCLASS(ClassGroup = (Nova))
class UNovaSoundSetup : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	UNovaSoundSetup() : MasterSoundMix(nullptr), MasterSoundClass(nullptr), MusicSoundClass(nullptr), EffectsSoundClass(nullptr)
	{}

public:
	// Musical tracks
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	TArray<FNovaMusicCatalogEntry> Tracks;

	// Master sound mixer
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	class USoundMix* MasterSoundMix;

	// Master sound class
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	class USoundClass* MasterSoundClass;

	// Music sound class
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	class USoundClass* MusicSoundClass;

	// Effects sound class
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	class USoundClass* EffectsSoundClass;
};

/*----------------------------------------------------
    Sound instance
----------------------------------------------------*/

// Sound state delegate
DECLARE_DELEGATE_RetVal(bool, FNovaSoundInstanceCallback);

/** Generic sound player with fading */
USTRUCT()
struct FNovaSoundInstance
{
	GENERATED_BODY()

public:
	FNovaSoundInstance() : SoundComponent(nullptr), StateCallback(), SoundPitchFade(false), SoundFadeSpeed(0.0f), CurrentVolume(0.0f)
	{}

	FNovaSoundInstance(UObject* Owner, FNovaSoundInstanceCallback Callback, class USoundBase* Sound = nullptr,
		bool ChangePitchWithFade = false, float FadeSpeed = 1.0f);

	/** Tick */
	void Update(float DeltaTime);

	/** Check if the sound was set up correctly */
	bool IsValid();

	/** Check if the sound has stopped */
	bool IsIdle();

public:
	/** Sound component */
	UPROPERTY()
	class UAudioComponent* SoundComponent;

	/** Callback to check the player state */
	FNovaSoundInstanceCallback StateCallback;

	/** Should this sound change pitch with the volume */
	bool SoundPitchFade;

	/** Fade speed */
	float SoundFadeSpeed;

	/** Volume */
	float CurrentVolume;
};

/*----------------------------------------------------
    System
----------------------------------------------------*/

/** Sound & music management class */
UCLASS(ClassGroup = (Nova))
class UNovaSoundManager
	: public UObject
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UNovaSoundManager();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Start playing on a new level */
	void BeginPlay(class ANovaPlayerController* PC);

	/** Request a musical track */
	void SetMusicTrack(FName Track);

	/** Register a new sound with its condition and settings */
	void AddSound(class USoundBase* Sound, FNovaSoundInstanceCallback Callback, bool ChangePitchWithFade = false, float FadeSpeed = 1.0f);

	/** Set the master volume from 0 to 10 */
	void SetMasterVolume(int32 Volume);

	/** Set the music volume from 0 to 10 */
	void SetMusicVolume(int32 Volume);

	/** Set the music volume from 0 to 10 */
	void SetEffectsVolume(int32 Volume);

	/*----------------------------------------------------
	    Tick
	----------------------------------------------------*/

	virtual void              Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UNovaSoundManager, STATGROUP_Tickables);
	}
	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Game instance
	UPROPERTY()
	class UNovaGameInstance* OwningGameInstance;

	// General state
	FAudioDeviceHandle             AudioDevice;
	class ANovaPlayerController*   PlayerController;
	TMap<FName, class USoundBase*> MusicTracks;
	FName                          CurrentMusicTrack;
	FName                          DesiredMusicTrack;

	// Volume
	float MasterVolume;
	float MusicVolume;
	float EffectsVolume;
	float EffectsVolumeMultiplier;

	// Master sound mixer
	UPROPERTY()
	class USoundMix* MasterSoundMix;

	// Master sound class
	UPROPERTY()
	class USoundClass* MasterSoundClass;

	// Music sound class
	UPROPERTY()
	class USoundClass* MusicSoundClass;

	// Effects sound class
	UPROPERTY()
	class USoundClass* EffectsSoundClass;

	// Dedicated music player instance
	UPROPERTY()
	FNovaSoundInstance MusicInstance;

	// Sound player instances
	UPROPERTY()
	TArray<FNovaSoundInstance> SoundInstances;
};
