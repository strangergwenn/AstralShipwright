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

/** Data for a stable orbit that might be a circular, elliptical or Hohmann transfer orbit */
struct FNovaOrbit
{
	FNovaOrbit() : StartAltitude(0), OppositeAltitude(0), StartPhase(0), EndPhase(0)
	{}

	FNovaOrbit(float SA, float CP) : StartAltitude(SA), OppositeAltitude(SA), StartPhase(0), EndPhase(360)
	{}

	FNovaOrbit(float SA, float EA, float SP, float EP) : StartAltitude(SA), OppositeAltitude(EA), StartPhase(SP), EndPhase(EP)
	{}

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
	FNovaTrajectory(const FNovaOrbit& Orbit) : CurrentOrbit(Orbit), TotalTransferDuration(0), TotalDeltaV(0)
	{}

	FNovaOrbit            CurrentOrbit;
	TArray<FNovaOrbit>    TransferOrbits;
	TArray<FNovaManeuver> Maneuvers;
	FNovaOrbit            FinalOrbit;

	double TotalTransferDuration;
	double TotalDeltaV;
};

/** Point of interest on the map */
struct FNovaOrbitalObject
{
	FNovaOrbitalObject() : Area(nullptr), Spacecraft(nullptr), Maneuver(nullptr)
	{}

	FNovaOrbitalObject(const class UNovaArea* A, float P) : FNovaOrbitalObject()
	{
		Area  = A;
		Phase = P;
	}

	FNovaOrbitalObject(const TSharedPtr<struct FNovaSpacecraft>& S, float P) : FNovaOrbitalObject()
	{
		Spacecraft = S;
		Phase      = P;
	}

	FNovaOrbitalObject(const FNovaManeuver& M) : FNovaOrbitalObject()
	{
		Maneuver = MakeShared<FNovaManeuver>(M);
		Phase    = M.Phase;
	}

	FText GetText() const;

	// Object data
	const class UNovaArea*             Area;
	TSharedPtr<struct FNovaSpacecraft> Spacecraft;
	TSharedPtr<FNovaManeuver>          Maneuver;

	// Positioning
	float     Phase;
	FVector2D Position;
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
