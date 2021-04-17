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

	virtual void Update(double InitialTime, double FinalTime) override;

	/** Get how much propellant remains available in T */
	float GetAvailablePropellantAmount() const
	{
		const FNovaSpacecraftPropulsionMetrics* PropulsionMetrics = GetPropulsionMetrics();

		if (PropulsionMetrics)
		{
			return PropulsionMetrics->PropellantMass - GetConsumedPropellantAmount();
		}
		else
		{
			return 0;
		}
	}

	/** Get how much propellant has been consumed in T */
	float GetConsumedPropellantAmount() const
	{
		return ConsumedAmount;
	}

	/** Get how much propellant can be stored in T */
	float GetTotalPropellantAmount() const
	{
		const FNovaSpacecraftPropulsionMetrics* PropulsionMetrics = GetPropulsionMetrics();
		return PropulsionMetrics ? PropulsionMetrics->PropellantMass : 0;
	}

	/** Return the current propellant mass rate in T/s */
	float GetCurrentPropellantRate() const
	{
		return CurrentRate;
	}

	/** Reset completely the consumed amount */
	void Refill()
	{
		NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
		ConsumedAmount = 0;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Current rate of consumption
	UPROPERTY(Replicated)
	float CurrentRate;

	// Current consumed amount
	UPROPERTY(Replicated)
	float ConsumedAmount;
};
