// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"

#include "NovaOrbitalSimulationComponent.generated.h"

/** Hohmann transfer orbit parameters */
struct FNovaHohmannTransfer
{
	double StartDeltaV;
	double EndDeltaV;
	double TotalDeltaV;
	double Duration;
};

/** Orbital simulation component that ticks orbiting spacecraft */
UCLASS(ClassGroup = (Nova))
class UNovaOrbitalSimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaOrbitalSimulationComponent();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Compute a trajectory */
	TSharedPtr<FNovaTrajectory> ComputeTrajectory(const class UNovaArea* Source, const class UNovaArea* Destination, float PhasingAltitude);

	/** Get the trajectory data */
	TMap<const class UNovaArea*, FNovaTrajectory> GetTrajectories() const
	{
		return Trajectories;
	}

	/** Get the position data */
	TMap<const class UNovaArea*, float> GetPositions() const
	{
		return Positions;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update all trajectories */
	void UpdateTrajectories();

	/** Update all positions based on the current time */
	void UpdatePositions();

	/** Compute the parameters of a Hohmann transfer orbit */
	static FNovaHohmannTransfer ComputeHohmannTransfer(
		const double GravitationalParameter, const double SourceRadius, const double DestinationRadius)
	{
		FNovaHohmannTransfer Result;

		Result.StartDeltaV =
			abs(sqrt(GravitationalParameter / SourceRadius) * (sqrt((2.0 * DestinationRadius) / (SourceRadius + DestinationRadius)) - 1.0));
		Result.EndDeltaV =
			abs(sqrt(GravitationalParameter / DestinationRadius) * (1.0 - sqrt((2.0 * SourceRadius) / (SourceRadius + DestinationRadius))));

		Result.TotalDeltaV = Result.StartDeltaV + Result.EndDeltaV;

		Result.Duration = PI * sqrt(pow(SourceRadius + DestinationRadius, 3.0) / (8.0 * GravitationalParameter)) / 60;

		return Result;
	}

	/** Compute the period of a stable circular orbit */
	static double GetCircularOrbitPeriod(const double GravitationalParameter, const double Radius)
	{
		return 2.0 * PI * sqrt(pow(Radius, 3.0) / GravitationalParameter) / 60.0;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Local state
	TArray<const class UNovaArea*>                Areas;
	TMap<const class UNovaArea*, FNovaTrajectory> Trajectories;
	TMap<const class UNovaArea*, float>           Positions;
};
