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
		const FNovaSpacecraft* Spacecraft = GetSpacecraft();
		return Spacecraft ? Spacecraft->GetPowerMetrics().EnergyCapacity : 0.0;
	}

	/** Get the current power delta */
	double GetCurrentPower() const
	{
		return CurrentPower;
	}

	/** Get the total current power production */
	double GetCurrentProduction() const
	{
		return CurrentPowerProduction;
	}

	/** Get the total current power consumption */
	double GetCurrentConsumption() const
	{
		return CurrentPowerConsumption;
	}

	/** Get the minimum (in relative terms) total power delta */
	double GetMinimumPower() const
	{
		const FNovaSpacecraft* Spacecraft = GetSpacecraft();
		return Spacecraft ? -Spacecraft->GetPowerMetrics().TotalPowerUsage : 0.0;
	}

	/** Get the maximum power delta */
	double GetMaximumPower() const
	{
		const FNovaSpacecraft* Spacecraft = GetSpacecraft();
		return Spacecraft ? Spacecraft->GetPowerMetrics().TotalPowerProduction : 0.0;
	}

	/** Get the total power delta range */
	double GetPowerRange() const
	{
		return GetMaximumPower() - GetMinimumPower();
	}

	/** Get the current solar exposure ratio */
	double GetSolarExposure() const
	{
		return CurrentExposureRatio;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Current power delta
	UPROPERTY(Replicated)
	double CurrentPower;

	// Current power production
	UPROPERTY(Replicated)
	double CurrentPowerProduction;

	// Current power consumption
	UPROPERTY(Replicated)
	double CurrentPowerConsumption;

	// Current solar exposure
	UPROPERTY(Replicated)
	double CurrentExposureRatio;

	// Current battery state
	UPROPERTY(Replicated)
	double CurrentEnergy;

	// Current battery capacity
	UPROPERTY(Replicated)
	double EnergyCapacity;
};
