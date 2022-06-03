// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "NovaAssetManager.h"
#include "NovaSoundManager.generated.h"

/*----------------------------------------------------
    Music catalog
----------------------------------------------------*/

// Music getter
DECLARE_DELEGATE_RetVal(FName, FNovaMusicCallback);

// Soundtrack list entry
USTRUCT()
struct FNovaMusicCatalogEntry
{
	GENERATED_BODY()

	FNovaMusicCatalogEntry() : Name(NAME_None)
	{}

	/** Track name */
	UPROPERTY(Category = Sound, EditDefaultsOnly)
	FName Name;

	/** Sound asset */
	UPROPERTY(Category = Sound, EditDefaultsOnly)
	TArray<class USoundBase*> Tracks;
};

// Environment sound entry
USTRUCT()
struct FNovaEnvironmentSoundEntry
{
	GENERATED_BODY()

	FNovaEnvironmentSoundEntry() : Sound(nullptr), ChangePitchWithFade(true), SoundFadeSpeed(1.0f)
	{}

	/** Sound asset */
	UPROPERTY(Category = Sound, EditDefaultsOnly)
	class USoundBase* Sound;

	/** Whether to fade the pitch */
	UPROPERTY(Category = Sound, EditDefaultsOnly)
	bool ChangePitchWithFade;

	/** Sound asset */
	UPROPERTY(Category = Sound, EditDefaultsOnly)
	float SoundFadeSpeed;
};

// Music catalog
UCLASS(ClassGroup = (Nova))
class UNovaSoundSetup : public UNovaAssetDescription
{
	GENERATED_BODY()

public:

	UNovaSoundSetup()
		: MusicFadeSpeed(2.0f)
		, FadeEffectsInMenus(false)
		, MasterSoundMix(nullptr)
		, MasterSoundClass(nullptr)
		, UISoundClass(nullptr)
		, EffectsSoundClass(nullptr)
		, MusicSoundClass(nullptr)
	{}

public:

	// Musical tracks
	UPROPERTY(Category = Music, EditDefaultsOnly)
	TArray<FNovaMusicCatalogEntry> Tracks;

	// Music fade speed
	UPROPERTY(Category = Music, EditDefaultsOnly)
	float MusicFadeSpeed;

	// Environment sounds
	UPROPERTY(Category = Environment, EditDefaultsOnly)
	TMap<FName, FNovaEnvironmentSoundEntry> Sounds;

	// Fade out effect sounds in menus
	UPROPERTY(Category = Environment, EditDefaultsOnly)
	bool FadeEffectsInMenus;

	// Master sound mixer
	UPROPERTY(Category = Mixing, EditDefaultsOnly)
	class USoundMix* MasterSoundMix;

	// Master sound class
	UPROPERTY(Category = Mixing, EditDefaultsOnly)
	class USoundClass* MasterSoundClass;

	// UI sound class
	UPROPERTY(Category = Mixing, EditDefaultsOnly)
	class USoundClass* UISoundClass;

	// Effects sound class
	UPROPERTY(Category = Mixing, EditDefaultsOnly)
	class USoundClass* EffectsSoundClass;

	// Music sound class
	UPROPERTY(Category = Mixing, EditDefaultsOnly)
	class USoundClass* MusicSoundClass;
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
	    System interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNovaSoundManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNovaGameInstance* GameInstance)
	{
		Singleton = this;
	}

	/** Start playing on a new level */
	void BeginPlay(class ANovaPlayerController* PC, FNovaMusicCallback Callback);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Mute all sounds */
	void Mute();

	/** Restore all sounds */
	void UnMute();

	/** Register a new sound with its condition and settings */
	void AddEnvironmentSound(
		FName SoundName, FNovaSoundInstanceCallback Callback, bool ChangePitchWithFade = false, float FadeSpeed = 1.0f);

	/** Set the master volume from 0 to 10 */
	void SetMasterVolume(int32 Volume);

	/** Set the UI volume from 0 to 10 */
	void SetUIVolume(int32 Volume);

	/** Set the music volume from 0 to 10 */
	void SetEffectsVolume(int32 Volume);

	/** Set the music volume from 0 to 10 */
	void SetMusicVolume(int32 Volume);

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

	// Singleton pointer
	static UNovaSoundManager* Singleton;

	// Player owner
	UPROPERTY()
	class ANovaPlayerController* PlayerController;

	// Sound setup
	UPROPERTY()
	const UNovaSoundSetup* SoundSetup;

	// General state
	FAudioDeviceHandle                     AudioDevice;
	TMap<FName, TArray<class USoundBase*>> MusicCatalog;
	FNovaMusicCallback                     MusicCallback;
	FName                                  CurrentMusicTrack;
	FName                                  DesiredMusicTrack;

	// Volume
	float MasterVolume;
	float UIVolume;
	float EffectsVolume;
	float EffectsVolumeMultiplier;
	float MusicVolume;

	// Dedicated music player instance
	UPROPERTY()
	FNovaSoundInstance MusicSoundInstance;

	// Environment player instances
	UPROPERTY()
	TArray<FNovaSoundInstance> EnvironmentSoundInstances;
};
