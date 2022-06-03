// Astral Shipwright - Gwennaël Arbona

#include "NovaPostProcessManager.h"
#include "Game/Settings/NovaGameUserSettings.h"
#include "Player/NovaPlayerController.h"
#include "UI/NovaUI.h"
#include "Nova.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine.h"

// Statics
UNovaPostProcessManager* UNovaPostProcessManager::Singleton = nullptr;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaPostProcessManager::UNovaPostProcessManager()
	: Super(), PlayerController(nullptr), CurrentPreset(0), TargetPreset(0), CurrentPresetAlpha(0.0f)
{}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void UNovaPostProcessManager::Initialize(UNovaGameInstance* GameInstance)
{
	Singleton = this;
}

void UNovaPostProcessManager::BeginPlay(ANovaPlayerController* PC, FNovaPostProcessControl Control, FNovaPostProcessUpdate Update)
{
	PlayerController = PC;
	ControlFunction  = Control;
	UpdateFunction   = Update;

	NCHECK(PlayerController);
	if (PlayerController->IsLocalController())
	{
		TArray<AActor*> PostProcessActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPostProcessActor::StaticClass(), PostProcessActors);

		// Set the post process
		if (PostProcessActors.Num())
		{
			PostProcessVolume =
				Cast<UPostProcessComponent>(PostProcessActors[0]->GetComponentByClass(UPostProcessComponent::StaticClass()));

			// Replace the material by a dynamic variant
			if (PostProcessVolume)
			{
				TArray<FWeightedBlendable> Blendables = PostProcessVolume->Settings.WeightedBlendables.Array;
				PostProcessVolume->Settings.WeightedBlendables.Array.Empty();

				for (FWeightedBlendable Blendable : Blendables)
				{
					UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(Blendable.Object);
					NCHECK(BaseMaterial);

					UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, GetWorld());
					if (!IsValid(PostProcessMaterial))
					{
						PostProcessMaterial = MaterialInstance;
					}

					PostProcessVolume->Settings.AddBlendable(MaterialInstance, 1.0f);
				}

				NLOG("UNovaPostProcessManager::BeginPlay : post-process setup complete");
			}
		}
	}
}

/*----------------------------------------------------
    Tick
----------------------------------------------------*/

void UNovaPostProcessManager::Tick(float DeltaTime)
{
	if (IsValid(PlayerController) && PlayerController->IsLocalController() && PostProcessVolume)
	{
		// Update desired settings
		if (ControlFunction.IsBound() && CurrentPreset == TargetPreset)
		{
			TargetPreset = ControlFunction.Execute();
		}

		// Update transition time
		float CurrentTransitionDuration = PostProcessSettings[TargetPreset]->TransitionDuration;
		if (CurrentPreset != TargetPreset)
		{
			CurrentPresetAlpha -= DeltaTime / CurrentTransitionDuration;
		}
		else if (CurrentPreset != 0)
		{
			CurrentPresetAlpha += DeltaTime / CurrentTransitionDuration;
		}
		CurrentPresetAlpha = FMath::Clamp(CurrentPresetAlpha, 0.0f, 1.0f);

		// Manage state transitions
		if (CurrentPresetAlpha <= 0)
		{
			CurrentPreset = TargetPreset;
		}

		// Get post process targets and apply the new settings
		float InterpolatedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentPresetAlpha, ENovaUIConstants::EaseStandard);
		TSharedPtr<FNovaPostProcessSettingBase>& CurrentPostProcess = PostProcessSettings[0];
		TSharedPtr<FNovaPostProcessSettingBase>& TargetPostProcess  = PostProcessSettings[CurrentPreset];
		UpdateFunction.ExecuteIfBound(PostProcessVolume, PostProcessMaterial, CurrentPostProcess, TargetPostProcess, CurrentPresetAlpha);

		// Apply config-driven settings
		UNovaGameUserSettings* GameUserSettings                               = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
		PostProcessVolume->Settings.bOverride_BloomMethod                     = true;
		PostProcessVolume->Settings.bOverride_DynamicGlobalIlluminationMethod = true;
		PostProcessVolume->Settings.bOverride_ReflectionMethod                = true;
		PostProcessVolume->Settings.BloomMethod                               = GameUserSettings->EnableCinematicBloom ? BM_FFT : BM_SOG;
		PostProcessVolume->Settings.DynamicGlobalIlluminationMethod =
			GameUserSettings->EnableLumen ? EDynamicGlobalIlluminationMethod::Lumen : EDynamicGlobalIlluminationMethod::ScreenSpace;
		PostProcessVolume->Settings.ReflectionMethod =
			GameUserSettings->EnableLumen ? EReflectionMethod::Lumen : EReflectionMethod::ScreenSpace;
	}
}
