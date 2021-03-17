// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "NovaArea.h"
#include "NovaOrbitalSimulationTypes.h"
#include "Nova/Game/NovaGameTypes.h"

#include "NovaOrbitalSimulationComponent.generated.h"

/** Orbital simulation component that ticks orbiting spacecraft */
UCLASS(ClassGroup = (Nova))
class UNovaOrbitalSimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaOrbitalSimulationComponent();

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*----------------------------------------------------
	    Trajectory & orbiting interface
	----------------------------------------------------*/

public:
	/** Build trajectory parameters */
	FNovaTrajectoryParameters PrepareTrajectory(const UNovaArea* Source, const UNovaArea* Destination, double DeltaTime = 0) const;

	/** Compute a trajectory */
	TSharedPtr<FNovaTrajectory> ComputeTrajectory(const FNovaTrajectoryParameters& Parameters, float PhasingAltitude);

	/** Check if this trajectory can be started */
	bool CanStartTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory) const;

	/** Put spacecraft on a new trajectory */
	void StartTrajectory(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory);

	/** Complete the current trajectory of spacecraft */
	void CompleteTrajectory(const TArray<FGuid>& SpacecraftIdentifiers);

	/** Put spacecraft in a particular orbit */
	void SetOrbit(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit);

	/** Merge different spacecraft in a particular orbit */
	void MergeOrbit(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit);

	/*----------------------------------------------------
	    Trajectory & orbiting getters
	----------------------------------------------------*/

	/** Get this component */
	static UNovaOrbitalSimulationComponent* Get(const UObject* Outer);

	/** Get the orbital data for an area */
	TSharedPtr<FNovaOrbit> GetAreaOrbit(const UNovaArea* Area) const
	{
		return MakeShared<FNovaOrbit>(FNovaOrbitGeometry(Area->Planet, Area->Altitude, Area->Phase), 0);
	}

	/** Get an area's location */
	const FNovaOrbitalLocation& GetAreaLocation(const UNovaArea* Area) const
	{
		return AreaOrbitalLocations[Area];
	}

	/** Get all area's locations */
	const TMap<const UNovaArea*, FNovaOrbitalLocation>& GetAllAreasLocations() const
	{
		return AreaOrbitalLocations;
	}

	/** Get the closest area and the associated distance from an arbitrary location */
	TPair<const UNovaArea*, float> GetClosestAreaAndDistance(const FNovaOrbitalLocation& Location) const;

	/** Get a spacecraft's orbit */
	const FNovaOrbit* GetSpacecraftOrbit(const FGuid& Identifier) const
	{
		return SpacecraftOrbitDatabase.Get(Identifier);
	}

	/** Get a spacecraft's trajectory */
	const FNovaTrajectory* GetSpacecraftTrajectory(const FGuid& Identifier) const
	{
		return SpacecraftTrajectoryDatabase.Get(Identifier);
	}

	/** Get a spacecraft's location */
	const FNovaOrbitalLocation* GetSpacecraftLocation(const FGuid& Identifier) const
	{
		return &SpacecraftOrbitalLocations[Identifier];
	}

	/** Get all spacecraft's locations */
	const TMap<FGuid, FNovaOrbitalLocation>& GetAllSpacecraftLocations() const
	{
		return SpacecraftOrbitalLocations;
	}

	/** Get the player orbit */
	const FNovaOrbit* GetPlayerOrbit() const;

	/** Get the player trajectory */
	const FNovaTrajectory* GetPlayerTrajectory() const;

	/** Get the player location */
	const FNovaOrbitalLocation* GetPlayerLocation() const;

	/** Get the current time in minutes */
	double GetCurrentTime() const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update all area's position */
	void ProcessAreas();

	/** Update the current orbit of spacecraft */
	void ProcessSpacecraftOrbits();

	/** Update the current trajectory of spacecraft */
	void ProcessSpacecraftTrajectories();

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

	/** Get the current phase of an orbit */
	double GetCircularOrbitPhase(const UNovaPlanet* Planet, const FNovaOrbit& Orbit, double DeltaTime = 0) const
	{
		const double OrbitalPeriod =
			GetCircularOrbitPeriod(Planet->GetGravitationalParameter(), Planet->GetRadius(Orbit.Geometry.StartAltitude));
		const double CurrentTime = GetCurrentTime() + DeltaTime;
		return FMath::Fmod(Orbit.Geometry.StartPhase + (CurrentTime / OrbitalPeriod) * 360, 360.0);
	}

	/** Get the current phase of an area */
	double GetAreaPhase(const UNovaArea* Area, double DeltaTime = 0) const
	{
		const double OrbitalPeriod =
			GetCircularOrbitPeriod(Area->Planet->GetGravitationalParameter(), Area->Planet->GetRadius(Area->Altitude));
		const double CurrentTime = GetCurrentTime() + DeltaTime;
		return FMath::Fmod(Area->Phase + (CurrentTime / OrbitalPeriod) * 360, 360.0);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Replicated orbit database
	UPROPERTY(Replicated) FNovaOrbitDatabase SpacecraftOrbitDatabase;

	// Replicated trajectory database
	UPROPERTY(Replicated)
	FNovaTrajectoryDatabase SpacecraftTrajectoryDatabase;

	// Local state
	const class ANovaPlayerState*                      CurrentPlayerState;
	TArray<const class UNovaArea*>                     Areas;
	TMap<const class UNovaArea*, FNovaOrbitalLocation> AreaOrbitalLocations;
	TMap<FGuid, FNovaOrbitalLocation>                  SpacecraftOrbitalLocations;
};
