// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Game/NovaGameTypes.h"

#include "NovaOrbitalSimulationTypes.generated.h"

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
USTRUCT(Atomic)
struct FNovaOrbit
{
	GENERATED_BODY()

	FNovaOrbit() : Planet(nullptr), StartAltitude(0), OppositeAltitude(0), StartPhase(0), EndPhase(0)
	{}

	FNovaOrbit(const class UNovaPlanet* P, float SA, float SP)
		: Planet(P), StartAltitude(SA), OppositeAltitude(SA), StartPhase(SP), EndPhase(SP + 360)
	{
		NCHECK(Planet != nullptr);
	}

	FNovaOrbit(const class UNovaPlanet* P, float SA, float EA, float SP, float EP)
		: Planet(P), StartAltitude(SA), OppositeAltitude(EA), StartPhase(SP), EndPhase(EP)
	{
		NCHECK(Planet != nullptr);
	}

	bool operator==(const FNovaOrbit& Other) const
	{
		return StartAltitude == Other.StartAltitude && OppositeAltitude == Other.OppositeAltitude && StartPhase == Other.StartPhase &&
			   EndPhase == Other.EndPhase;
	}

	/** Check for validity */
	bool IsValid() const
	{
		return Planet != nullptr && StartAltitude > 0 && OppositeAltitude > 0;
	}

	/** Get the maximum altitude reached by this orbit */
	float GetHighestAltitude() const
	{
		return FMath::Max(StartAltitude, OppositeAltitude);
	}

	UPROPERTY()
	const class UNovaPlanet* Planet;

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
struct FNovaTimedOrbit
{
	GENERATED_BODY()

	FNovaTimedOrbit() : Orbit(), InsertionTime(0)
	{}

	FNovaTimedOrbit(const FNovaOrbit& O, float T) : Orbit(O), InsertionTime(T)
	{
		NCHECK(Orbit.Planet != nullptr);
	}

	bool operator==(const FNovaTimedOrbit& Other) const
	{
		return Orbit == Other.Orbit && InsertionTime == Other.InsertionTime;
	}

	/** Check for validity */
	bool IsValid() const
	{
		return Orbit.IsValid();
	}

	UPROPERTY()
	FNovaOrbit Orbit;

	UPROPERTY()
	float InsertionTime;
};

/** Current location of a spacecraft or sector, including orbit + phase */
USTRUCT(Atomic)
struct FNovaOrbitalLocation
{
	GENERATED_BODY()

	FNovaOrbitalLocation() : Orbit(), Phase(0)
	{}

	FNovaOrbitalLocation(const FNovaOrbit& O, float P) : Orbit(O), Phase(P)
	{}

	UPROPERTY()
	FNovaOrbit Orbit;

	UPROPERTY()
	float Phase;
};

/*** Orbit-altering maneuver */
USTRUCT(Atomic)
struct FNovaManeuver
{
	GENERATED_BODY()

	FNovaManeuver() : DeltaV(0), Time(0), Phase(0)
	{}

	FNovaManeuver(float DV, float T, float P) : DeltaV(DV), Time(T), Phase(P)
	{}

	UPROPERTY()
	float DeltaV;

	UPROPERTY()
	float Time;

	UPROPERTY()
	float Phase;
};

/** Full trajectory data including the last stable orbit */
USTRUCT()
struct FNovaTrajectory
{
	GENERATED_BODY()

	FNovaTrajectory() : TotalTransferDuration(0), TotalDeltaV(0)
	{}

	bool operator==(const FNovaTrajectory& Other) const
	{
		return TransferOrbits.Num() == Other.TransferOrbits.Num() && Maneuvers.Num() == Other.Maneuvers.Num() &&
			   TotalTransferDuration == Other.TotalTransferDuration && TotalDeltaV == Other.TotalDeltaV;
	}

	/** Check for validity */
	bool IsValid() const
	{
		return TransferOrbits.Num() > 0 && Maneuvers.Num() > 0;
	}

	/** Get the maximum altitude reached by this trajectory */
	float GetHighestAltitude() const;

	/** Compute the final orbit this trajectory will put the spacecraft in */
	FNovaTimedOrbit GetFinalOrbit() const;

	UPROPERTY()
	TArray<FNovaOrbit> TransferOrbits;

	UPROPERTY()
	TArray<FNovaManeuver> Maneuvers;

	UPROPERTY()
	float TotalTransferDuration;

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
	FNovaTimedOrbit Orbit;

	UPROPERTY()
	TArray<FGuid> Identifiers;
};

/** Orbit database with fast array replication and fast lookup */
USTRUCT()
struct FNovaOrbitDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool AddOrUpdate(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTimedOrbit>& Orbit)
	{
		NCHECK(Orbit.IsValid());
		NCHECK(Orbit->IsValid());

		FNovaOrbitDatabaseEntry TrajectoryData;
		TrajectoryData.Orbit       = *Orbit;
		TrajectoryData.Identifiers = SpacecraftIdentifiers;

		return Cache.AddOrUpdate(*this, Array, TrajectoryData);
	}

	void Remove(const TArray<FGuid>& SpacecraftIdentifiers)
	{
		Cache.Remove(*this, Array, SpacecraftIdentifiers);
	}

	const FNovaTimedOrbit* Get(const FGuid& Identifier) const
	{
		const FNovaOrbitDatabaseEntry* Entry = Cache.Get(Identifier);
		return Entry ? &Entry->Orbit : nullptr;
	}

	const TArray<FNovaOrbitDatabaseEntry>& Get() const
	{
		return Array;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaOrbitDatabaseEntry, FNovaOrbitDatabase>(Array, DeltaParms, *this);
	}

	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
	{
		Cache.Update(Array);
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

	bool AddOrUpdate(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory)
	{
		NCHECK(Trajectory.IsValid());
		NCHECK(Trajectory->IsValid());

		FNovaTrajectoryDatabaseEntry TrajectoryData;
		TrajectoryData.Trajectory  = *Trajectory;
		TrajectoryData.Identifiers = SpacecraftIdentifiers;

		return Cache.AddOrUpdate(*this, Array, TrajectoryData);
	}

	void Remove(const TArray<FGuid>& SpacecraftIdentifiers)
	{
		Cache.Remove(*this, Array, SpacecraftIdentifiers);
	}

	const FNovaTrajectory* Get(const FGuid& Identifier) const
	{
		const FNovaTrajectoryDatabaseEntry* Entry = Cache.Get(Identifier);
		return Entry ? &Entry->Trajectory : nullptr;
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

	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
	{
		Cache.Update(Array);
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
