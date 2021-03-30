// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaArea.h"
#include "NovaGameTypes.h"

#include "NovaOrbitalSimulationTypes.generated.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

/** Data for a stable orbit that might be a circular, elliptical or Hohmann transfer orbit */
USTRUCT(Atomic)
struct FNovaOrbitGeometry
{
	GENERATED_BODY()

	FNovaOrbitGeometry() : Planet(nullptr), StartAltitude(0), OppositeAltitude(0), StartPhase(0), EndPhase(0)
	{}

	FNovaOrbitGeometry(const UNovaPlanet* P, float SA, float SP)
		: Planet(P), StartAltitude(SA), OppositeAltitude(SA), StartPhase(SP), EndPhase(SP + 360)
	{
		NCHECK(Planet != nullptr);
	}

	FNovaOrbitGeometry(const UNovaPlanet* P, float SA, float EA, float SP, float EP)
		: Planet(P), StartAltitude(SA), OppositeAltitude(EA), StartPhase(SP), EndPhase(EP)
	{
		NCHECK(Planet != nullptr);
	}

	bool operator==(const FNovaOrbitGeometry& Other) const
	{
		return StartAltitude == Other.StartAltitude && OppositeAltitude == Other.OppositeAltitude && StartPhase == Other.StartPhase &&
			   EndPhase == Other.EndPhase;
	}

	bool operator!=(const FNovaOrbitGeometry& Other) const
	{
		return !operator==(Other);
	}

	/** Check for validity */
	bool IsValid() const
	{
		return ::IsValid(Planet) && StartAltitude > 0 && OppositeAltitude > 0 && StartPhase >= 0 && EndPhase > 0 && StartPhase < EndPhase;
	}

	/** Check whether this geometry is circular */
	bool IsCircular() const
	{
		return StartAltitude == OppositeAltitude;
	}

	/** Get the maximum altitude reached by this orbit */
	float GetHighestAltitude() const
	{
		return FMath::Max(StartAltitude, OppositeAltitude);
	}

	/** Compute the period of this orbit geometry in minutes */
	double GetOrbitalPeriod() const
	{
		NCHECK(Planet);
		const double RadiusA = Planet->GetRadius(StartAltitude);
		const double RadiusB = Planet->GetRadius(OppositeAltitude);
		const double µ       = Planet->GetGravitationalParameter();

		const float SemiMajorAxis = 0.5f * (RadiusA + RadiusB);
		return 2.0 * PI * sqrt(pow(SemiMajorAxis, 3.0) / µ) / 60.0;
	}

	/** Get the current phase on this orbit */
	template <bool Unwind>
	double GetCurrentPhase(const double DeltaTime) const
	{
		NCHECK(Planet);
		const double PhaseDelta = (DeltaTime / GetOrbitalPeriod()) * 360;
		double       Result     = StartPhase + (Unwind ? FMath::Fmod(PhaseDelta, 360.0) : PhaseDelta);

		if (Unwind)
		{
			NCHECK(Result < StartPhase + 360);
		}

		return Result;
	}

	UPROPERTY()
	const UNovaPlanet* Planet;

	UPROPERTY()
	float StartAltitude;

	UPROPERTY()
	float OppositeAltitude;

	UPROPERTY()
	float StartPhase;

	UPROPERTY()
	float EndPhase;
};

/** Orbit + time of insertion, allowing prediction of where a spacecraft will be at any time */
USTRUCT(Atomic)
struct FNovaOrbit
{
	GENERATED_BODY()

	FNovaOrbit() : Geometry(), InsertionTime(0)
	{}

	FNovaOrbit(const FNovaOrbitGeometry& G, double T) : Geometry(G), InsertionTime(T)
	{
		NCHECK(Geometry.Planet != nullptr);
	}

	bool operator==(const FNovaOrbit& Other) const
	{
		return Geometry == Other.Geometry && InsertionTime == Other.InsertionTime;
	}

	bool operator!=(const FNovaOrbit& Other) const
	{
		return !operator==(Other);
	}

	/** Check for validity */
	bool IsValid() const
	{
		return Geometry.IsValid() && InsertionTime >= 0;
	}

	/** Get the current phase on this orbit */
	template <bool Unwind>
	double GetCurrentPhase(double CurrentTime) const
	{
		return Geometry.GetCurrentPhase<Unwind>(CurrentTime - InsertionTime);
	}

	UPROPERTY()
	FNovaOrbitGeometry Geometry;

	UPROPERTY()
	double InsertionTime;
};

/** Current location of a spacecraft or sector, including orbit + phase */
USTRUCT(Atomic)
struct FNovaOrbitalLocation
{
	GENERATED_BODY()

	FNovaOrbitalLocation() : Geometry(), Phase(0)
	{}

	FNovaOrbitalLocation(const FNovaOrbitGeometry& G, float P) : Geometry(G), Phase(P)
	{}

	/** Check for validity */
	bool IsValid() const
	{
		return Geometry.IsValid() && Phase >= 0;
	}

	/** Get the linear distance between this location and another */
	float GetDistanceTo(const FNovaOrbitalLocation& Other) const
	{
		return (GetCartesianLocation() - Other.GetCartesianLocation()).Size();
	}

	/** Get the Cartesian coordinates for this location */
	FVector2D GetCartesianLocation() const
	{
		return FVector2D(0.5f * (Geometry.OppositeAltitude - Geometry.StartAltitude), 0).GetRotated(Geometry.StartPhase) +
			   FVector2D(Geometry.StartAltitude, 0).GetRotated(Phase);
	}

	UPROPERTY()
	FNovaOrbitGeometry Geometry;

	UPROPERTY()
	float Phase;
};

/*** Orbit-altering maneuver */
USTRUCT(Atomic)
struct FNovaManeuver
{
	GENERATED_BODY()

	FNovaManeuver() : DeltaV(0), Phase(0), Time(0), Duration(0)
	{}

	FNovaManeuver(float DV, float P, double T, float D) : DeltaV(DV), Phase(P), Time(T), Duration(D)
	{}

	UPROPERTY()
	float DeltaV;

	UPROPERTY()
	float Phase;

	UPROPERTY()
	double Time;

	UPROPERTY()
	float Duration;
};

/** Full trajectory data including the last stable orbit */
USTRUCT()
struct FNovaTrajectory
{
	GENERATED_BODY()

	FNovaTrajectory() : TotalTravelDuration(0), TotalDeltaV(0)
	{}

	bool operator==(const FNovaTrajectory& Other) const
	{
		return Transfers.Num() == Other.Transfers.Num() && Maneuvers.Num() == Other.Maneuvers.Num() &&
			   TotalTravelDuration == Other.TotalTravelDuration && TotalDeltaV == Other.TotalDeltaV;
	}

	bool operator!=(const FNovaTrajectory& Other) const
	{
		return !operator==(Other);
	}

	/** Check for validity */
	bool IsValid() const
	{
		if (Transfers.Num() == 0 || Maneuvers.Num() == 0)
		{
			return false;
		}

		for (const FNovaOrbit& Transfer : Transfers)
		{
			if (!Transfer.IsValid())
			{
				return false;
			}
		}

		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.DeltaV == 0)
			{
				return false;
			}
		}

		return true;
	}

	/** Add a maneuver if it's not zero Delta-V */
	bool Add(const FNovaManeuver& Maneuver)
	{
		if (Maneuver.DeltaV != 0)
		{
			Maneuvers.Add(Maneuver);
			return true;
		}
		return false;
	}

	/** Add a transfer if it's not zero angular length */
	bool Add(const FNovaOrbit& Orbit)
	{
		if (Orbit.IsValid())
		{
			Transfers.Add(Orbit);
			return true;
		}
		return false;
	}

	/** Get the maximum altitude reached by this trajectory */
	float GetHighestAltitude() const;

	/** Compute the final orbit this trajectory will put the spacecraft in */
	FNovaOrbit GetFinalOrbit() const;

	/** Get the start time */
	double GetStartTime() const
	{
		NCHECK(IsValid());
		return GetArrivalTime() - static_cast<double>(TotalTravelDuration);
	}

	/** Get the start time of the first maneuver */
	double GetFirstManeuverStartTime() const
	{
		NCHECK(IsValid());
		return Maneuvers[0].Time;
	}

	/** Get the start time of the next maneuver */
	double GetNextManeuverStartTime(double CurrentTime) const
	{
		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time > CurrentTime)
			{
				return Maneuver.Time;
			}
		}

		return 0;
	}

	/** Get the arrival time */
	double GetArrivalTime() const
	{
		const FNovaManeuver& FinalManeuver = Maneuvers[Maneuvers.Num() - 1];
		return FinalManeuver.Time + static_cast<double>(FinalManeuver.Duration);
	}

	/** Get the number of remaining maneuvers */
	int32 GetRemainingManeuverCount(double CurrentTime) const
	{
		int32 Count = 0;

		for (const FNovaManeuver& Maneuver : Maneuvers)
		{
			if (Maneuver.Time > CurrentTime)
			{
				Count++;
			}
		}

		return Count;
	}

	/** Get the current location in orbit */
	FNovaOrbitalLocation GetCurrentLocation(double CurrentTime) const;

	/** Get the orbits that a maneuver is going from and to */
	TArray<FNovaOrbit> GetRelevantOrbitsForManeuver(const FNovaManeuver& Maneuver) const;

	UPROPERTY() TArray<FNovaOrbit> Transfers;

	UPROPERTY()
	TArray<FNovaManeuver> Maneuvers;

	UPROPERTY()
	float TotalTravelDuration;

	UPROPERTY()
	float TotalDeltaV;
};

/*----------------------------------------------------
    Orbit database
----------------------------------------------------*/

/** Replicated orbit information */
USTRUCT()
struct FNovaOrbitDatabaseEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	bool operator==(const FNovaOrbitDatabaseEntry& Other) const
	{
		return Orbit == Other.Orbit && Identifiers == Other.Identifiers;
	}

	UPROPERTY()
	FNovaOrbit Orbit;

	UPROPERTY()
	TArray<FGuid> Identifiers;
};

/** Orbit database with fast array replication and fast lookup */
USTRUCT()
struct FNovaOrbitDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit)
	{
		NCHECK(Orbit.IsValid());
		NCHECK(Orbit->IsValid());

		FNovaOrbitDatabaseEntry TrajectoryData;
		TrajectoryData.Orbit       = *Orbit;
		TrajectoryData.Identifiers = SpacecraftIdentifiers;

		return Cache.Add(*this, Array, TrajectoryData);
	}

	void Remove(const TArray<FGuid>& SpacecraftIdentifiers)
	{
		Cache.Remove(*this, Array, SpacecraftIdentifiers);
	}

	const FNovaOrbit* Get(const FGuid& Identifier) const
	{
		const FNovaOrbitDatabaseEntry* Entry = Cache.Get(Identifier, Array);
		return Entry ? &Entry->Orbit : nullptr;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	const TArray<FNovaOrbitDatabaseEntry>& Get() const
	{
		return Array;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaOrbitDatabaseEntry, FNovaOrbitDatabase>(Array, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FNovaOrbitDatabaseEntry> Array;

	TMultiGuidCacheMap<FNovaOrbitDatabaseEntry> Cache;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaOrbitDatabase> : public TStructOpsTypeTraitsBase2<FNovaOrbitDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/*----------------------------------------------------
    Trajectory database
----------------------------------------------------*/

/** Replicated trajectory information */
USTRUCT()
struct FNovaTrajectoryDatabaseEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	bool operator==(const FNovaTrajectoryDatabaseEntry& Other) const
	{
		return Trajectory == Other.Trajectory && Identifiers == Other.Identifiers;
	}

	UPROPERTY()
	FNovaTrajectory Trajectory;

	UPROPERTY()
	TArray<FGuid> Identifiers;
};

/** Trajectory database with fast array replication and fast lookup */
USTRUCT()
struct FNovaTrajectoryDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory)
	{
		NCHECK(Trajectory.IsValid());
		NCHECK(Trajectory->IsValid());

		FNovaTrajectoryDatabaseEntry TrajectoryData;
		TrajectoryData.Trajectory  = *Trajectory;
		TrajectoryData.Identifiers = SpacecraftIdentifiers;

		return Cache.Add(*this, Array, TrajectoryData);
	}

	void Remove(const TArray<FGuid>& SpacecraftIdentifiers)
	{
		Cache.Remove(*this, Array, SpacecraftIdentifiers);
	}

	const FNovaTrajectory* Get(const FGuid& Identifier) const
	{
		const FNovaTrajectoryDatabaseEntry* Entry = Cache.Get(Identifier, Array);
		return Entry ? &Entry->Trajectory : nullptr;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	const TArray<FNovaTrajectoryDatabaseEntry>& Get() const
	{
		return Array;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaTrajectoryDatabaseEntry, FNovaTrajectoryDatabase>(
			Array, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FNovaTrajectoryDatabaseEntry> Array;

	TMultiGuidCacheMap<FNovaTrajectoryDatabaseEntry> Cache;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaTrajectoryDatabase> : public TStructOpsTypeTraitsBase2<FNovaTrajectoryDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
