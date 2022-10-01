// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftSystemInterface.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftPowerSystem.generated.h"

/** Power system that simulates power usage */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftPowerSystem
	: public USceneComponent
	, public INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftPowerSystem();

	/*----------------------------------------------------
	    System implementation
	----------------------------------------------------*/

	virtual void Load(const FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftPowerSystem::Load");
	}

	virtual void Save(FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftPowerSystem::Save");
	}

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Get the current remaining energy */
	double GetRemainingEnergy() const
	{
		return 2;
	}

	/** Get the total storable energy */
	double GetEnergyCapacity() const
	{
		return 8;
	}

	/** Get the current power delta */
	double GetCurrentPower() const
	{
		return -1;
	}

	/** Get the maximum power delta */
	double GetMinimumPower() const
	{
		return -5;
	}

	/** Get the maximum power delta */
	double GetMaximumPower() const
	{
		return 2;
	}

	/** Get the total pwoer delta range */
	double GetPowerRange() const
	{
		return GetMaximumPower() - GetMinimumPower();
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
};
