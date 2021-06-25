// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftSystemInterface.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftPropellantSystem.generated.h"

/** Propellant system that tracks the usage of propellants by the spacecraft */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftPropellantSystem
	: public USceneComponent
	, public INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:
	UNovaSpacecraftPropellantSystem();

	/*----------------------------------------------------
	    System implementation
	----------------------------------------------------*/

	virtual void Load(const FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftPropellantSystem::Load : %f", Spacecraft.PropellantMassAtLaunch);

		PropellantMass = Spacecraft.PropellantMassAtLaunch;
	}

	virtual void Save(FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftPropellantSystem::Save : %f", PropellantMass);

		Spacecraft.PropellantMassAtLaunch = PropellantMass;
	}

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Get how much propellant remains available in T */
	float GetCurrentPropellantMass() const
	{
		if (IsSpacecraftDocked())
		{
			const FNovaSpacecraft* Spacecraft = GetSpacecraft();
			if (Spacecraft)
			{
				return Spacecraft->PropellantMassAtLaunch;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return PropellantMass;
		}
	}

	/** Get how much propellant can be stored in T */
	float GetPropellantCapacity() const
	{
		const FNovaSpacecraftPropulsionMetrics* PropulsionMetrics = GetPropulsionMetrics();
		return PropulsionMetrics ? PropulsionMetrics->PropellantMassCapacity : 0;
	}

	/** Return the current propellant mass rate in T/s */
	float GetCurrentPropellantRate() const
	{
		if (IsSpacecraftDocked())
		{
			return 0;
		}
		else
		{
			return PropellantRate;
		}
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Current rate of consumption
	UPROPERTY(Replicated)
	float PropellantRate;

	// Current propellant amount
	UPROPERTY(Replicated)
	float PropellantMass;
};
