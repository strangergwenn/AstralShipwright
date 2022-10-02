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

		CurrentEnergy  = Spacecraft.GetPowerMetrics().EnergyCapacity;
		EnergyCapacity = Spacecraft.GetPowerMetrics().EnergyCapacity;
		CurrentPower   = 0;
	}

	virtual void Save(FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftPowerSystem::Save");
	}

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Get the current remaining energy */
	double GetRemainingEnergy() const
	{
		return CurrentEnergy;
	}

	/** Get the total energy capacity in batteries */
	double GetEnergyCapacity() const
	{
		return GetSpacecraft()->GetPowerMetrics().EnergyCapacity;
	}

	/** Get the current power delta */
	double GetCurrentPower() const
	{
		return CurrentPower;
	}

	/** Get the minimum (in relative terms) total power delta */
	double GetMinimumPower() const
	{
		return -GetSpacecraft()->GetPowerMetrics().TotalPowerUsage;
	}

	/** Get the maximum power delta */
	double GetMaximumPower() const
	{
		return GetSpacecraft()->GetPowerMetrics().TotalPowerProduction;
	}

	/** Get the total power delta range */
	double GetPowerRange() const
	{
		return GetMaximumPower() - GetMinimumPower();
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Current power state
	UPROPERTY(Replicated)
	double CurrentPower;

	// Current battery state
	UPROPERTY(Replicated)
	double CurrentEnergy;

	// Current battery capacity
	UPROPERTY(Replicated)
	double EnergyCapacity;
};
