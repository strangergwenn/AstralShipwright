// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftSystemInterface.h"
#include "Game/NovaOrbitalSimulationComponent.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftProcessingSystem.generated.h"

/** Resource processing system that transforms resources */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftProcessingSystem
	: public USceneComponent
	, public INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftProcessingSystem();

	/*----------------------------------------------------
	    System implementation
	----------------------------------------------------*/

	virtual void Load(const FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftProcessingSystem::Load");
	}

	virtual void Save(FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftProcessingSystem::Save");
	}

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
};
