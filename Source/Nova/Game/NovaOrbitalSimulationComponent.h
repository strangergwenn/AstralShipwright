// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Nova/Game/NovaGameTypes.h"

#include "NovaOrbitalSimulationComponent.generated.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

/** Hohmann transfer orbit parameters */
struct FNovaHohmannTransfer
{
	double StartDeltaV;
	double EndDeltaV;
	double TotalDeltaV;
	double Duration;
};

/** Trajectory computation parameters */
struct FNovaTrajectoryParameters
{
	double StartTime;

	double SourceAltitude;
	double SourcePhase;
	double DestinationAltitude;
	double DestinationPhase;

	const class UNovaPlanet* Planet;
	double                   µ;
};

/** Data for a stable orbit that might be a circular, elliptical or Hohmann transfer orbit */
struct FNovaOrbit
{
	FNovaOrbit() : StartAltitude(0), OppositeAltitude(0), StartPhase(0), EndPhase(0)
	{}

	FNovaOrbit(float SA, float CP) : StartAltitude(SA), OppositeAltitude(SA), StartPhase(0), EndPhase(360)
	{}

	FNovaOrbit(float SA, float EA, float SP, float EP) : StartAltitude(SA), OppositeAltitude(EA), StartPhase(SP), EndPhase(EP)
	{}

	/** Get the maximum altitude reached by this orbit */
	float GetHighestAltitude() const
	{
		return FMath::Max(StartAltitude, OppositeAltitude);
	}

	float StartAltitude;
	float OppositeAltitude;
	float StartPhase;
	float EndPhase;
};

/*** Orbit-altering maneuver */
struct FNovaManeuver
{
	FNovaManeuver(double DV, double T, double P) : DeltaV(DV), Time(T), Phase(P)
	{}

	double DeltaV;
	double Time;
	double Phase;
};

/** Full trajectory data including the last stable orbit */
struct FNovaTrajectory
{
	FNovaTrajectory() : TotalTransferDuration(0), TotalDeltaV(0)
	{}

	/** Get the maximum altitude reached by this trajectory */
	float GetHighestAltitude() const;

	TArray<FNovaOrbit>    TransferOrbits;
	TArray<FNovaManeuver> Maneuvers;
	FNovaOrbit            FinalOrbit;

	double TotalTransferDuration;
	double TotalDeltaV;
};

/*----------------------------------------------------
    Simulator class
----------------------------------------------------*/

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

	/** Build trajectory parameters */
	FNovaTrajectoryParameters MakeTrajectoryParameters(
		const class UNovaArea* Source, const class UNovaArea* Destination, double DeltaTime = 0) const;

	/** Compute a trajectory */
	TSharedPtr<FNovaTrajectory> ComputeTrajectory(const FNovaTrajectoryParameters& Parameters, float PhasingAltitude);

	/** Get the orbit & position data for all areas */
	const TMap<const class UNovaArea*, TPair<FNovaOrbit, double>>& GetAreasOrbitAndPosition() const
	{
		return AreasOrbitAndPosition;
	}

	/** Get the current time in minutes */
	double GetCurrentTime() const
	{
		// TODO : shared time for multiplayer
		return GetWorld()->GetTimeSeconds() / 60.0;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update all area's position */
	void UpdateAreas();

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

	/** Compute the period of a stable circular orbit in minutes */
	static double GetCircularOrbitPeriod(const double GravitationalParameter, const double Radius)
	{
		return 2.0 * PI * sqrt(pow(Radius, 3.0) / GravitationalParameter) / 60.0;
	}

	/** Get the current phase of an area */
	double GetAreaPhase(const class UNovaArea* Area, double DeltaTime = 0) const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Local state
	TArray<const class UNovaArea*>                          Areas;
	TMap<const class UNovaArea*, TPair<FNovaOrbit, double>> AreasOrbitAndPosition;
};
