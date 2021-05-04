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

		PropellantAmount = Spacecraft.PropellantMassAtLaunch;
	}

	virtual void Save(FNovaSpacecraft& Spacecraft) override
	{
		NLOG("UNovaSpacecraftPropellantSystem::Save : %f", PropellantAmount);

		Spacecraft.PropellantMassAtLaunch = PropellantAmount;
	}

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Refill the spacecraft propellant, changing the system state directly */
	void Refill();

	UFUNCTION(Server, Reliable)
	void ServerRefill();

	/** Get how much propellant remains available in T */
	float GetCurrentPropellantAmount() const
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
			return PropellantAmount;
		}
	}

	/** Get how much propellant can be stored in T */
	float GetTotalPropellantAmount() const
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
	float PropellantAmount;
};
