// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NovaPostProcessComponent.generated.h"

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

/** Post-process handler component */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaPostProcessComponent();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Setup this post-process control component */
	void Initialize(FNovaPostProcessControl Control, FNovaPostProcessUpdate Update);

	/** Register a new preset - index 0 is considered neutral ! */
	template <typename T>
	void RegisterPreset(T Index, TSharedPtr<FNovaPostProcessSettingBase> Preset)
	{
		PostProcessSettings.Add(static_cast<int32>(Index), Preset);
	}

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	UPROPERTY()
	TSubclassOf<AActor> PostProcessActorClass;

	UPROPERTY()
	class UMaterialInstanceDynamic* PostProcessMaterial;

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
