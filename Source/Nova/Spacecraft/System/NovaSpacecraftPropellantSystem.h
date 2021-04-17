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

	virtual void Load(const FNovaSpacecraftSystemState& State) override
	{
		NLOG("UNovaSpacecraftPropellantSystem::Load");

		PropellantAmount = State.InitialPropellantMass;
	}

	virtual void Save(FNovaSpacecraftSystemState& State) override
	{
		NLOG("UNovaSpacecraftPropellantSystem::Save");

		State.InitialPropellantMass = PropellantAmount;
	}

	virtual void Update(double InitialTime, double FinalTime) override;

	/** Get how much propellant remains available in T */
	float GetCurrentPropellantAmount() const
	{
		if (IsSpacecraftDocked())
		{
			return GetSpacecraft()->SystemState.InitialPropellantMass;
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
		return PropulsionMetrics ? PropulsionMetrics->MaximumPropellantMass : 0;
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
