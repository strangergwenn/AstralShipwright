// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "GameFramework/Actor.h"

#include "NovaPostProcessManager.generated.h"

/*----------------------------------------------------
    Supporting types
----------------------------------------------------*/

// Base structure for post-process presets
struct FNovaPostProcessSettingBase
{
	FNovaPostProcessSettingBase() : TransitionDuration(0.5f)
	{}

	float TransitionDuration;
};

// Control function to determine the post-processing index
DECLARE_DELEGATE_RetVal(int32, FNovaPostProcessControl);
DECLARE_DELEGATE_FiveParams(FNovaPostProcessUpdate, class UPostProcessComponent*, class UMaterialInstanceDynamic*,
	const TSharedPtr<FNovaPostProcessSettingBase>&, const TSharedPtr<FNovaPostProcessSettingBase>&, float);

/** Post-processing owner */
UCLASS(ClassGroup = (Nova))
class ANovaPostProcessActor : public AActor
{
	GENERATED_BODY()
};

/*----------------------------------------------------
    System
----------------------------------------------------*/

/** Post-processing manager */
UCLASS(ClassGroup = (Nova))
class UNovaPostProcessManager
	: public UObject
	, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UNovaPostProcessManager();

	/*----------------------------------------------------
	    System interface
	----------------------------------------------------*/

	/** Get the singleton instance */
	static UNovaPostProcessManager* Get()
	{
		return Singleton;
	}

	/** Initialize this class */
	void Initialize(class UNovaGameInstance* GameInstance);

	/** Start playing on a new level */
	void BeginPlay(class ANovaPlayerController* PC, FNovaPostProcessControl Control, FNovaPostProcessUpdate Update);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Register a new preset - index 0 is considered neutral ! */
	template <typename T>
	void RegisterPreset(T Index, TSharedPtr<FNovaPostProcessSettingBase> Preset)
	{
		PostProcessSettings.Add(static_cast<int32>(Index), Preset);
	}

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

protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Singleton pointer
	static UNovaPostProcessManager* Singleton;

	// Post-process material that's dynamically controlled
	UPROPERTY()
	class UMaterialInstanceDynamic* PostProcessMaterial;

	// Post-process component that's dynamically controlled
	UPROPERTY()
	class UPostProcessComponent* PostProcessVolume;

	// General state
	FNovaPostProcessControl                              ControlFunction;
	FNovaPostProcessUpdate                               UpdateFunction;
	int32                                                CurrentPreset;
	int32                                                TargetPreset;
	float                                                CurrentPresetAlpha;
	TMap<int32, TSharedPtr<FNovaPostProcessSettingBase>> PostProcessSettings;
};
