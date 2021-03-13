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

	FNovaOrbit() : StartAltitude(0), OppositeAltitude(0), StartPhase(0), EndPhase(0)
	{}

	FNovaOrbit(float SA, float SP) : StartAltitude(SA), OppositeAltitude(SA), StartPhase(SP), EndPhase(SP + 360)
	{}

	FNovaOrbit(float SA, float EA, float SP, float EP) : StartAltitude(SA), OppositeAltitude(EA), StartPhase(SP), EndPhase(EP)
	{}

	bool operator==(const FNovaOrbit& Other) const
	{
		return StartAltitude == Other.StartAltitude && OppositeAltitude == Other.OppositeAltitude && StartPhase == Other.StartPhase &&
			   EndPhase == Other.EndPhase;
	}

	/** Get the maximum altitude reached by this orbit */
	float GetHighestAltitude() const
	{
		return FMath::Max(StartAltitude, OppositeAltitude);
	}

	UPROPERTY()
	float StartAltitude;

	UPROPERTY()
	float OppositeAltitude;

	UPROPERTY()
	float StartPhase;

	UPROPERTY()
	float EndPhase;
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
			   FinalOrbit == Other.FinalOrbit && TotalTransferDuration == Other.TotalTransferDuration && TotalDeltaV == Other.TotalDeltaV;
	}

	/** Get the maximum altitude reached by this trajectory */
	float GetHighestAltitude() const;

	UPROPERTY()
	TArray<FNovaOrbit> TransferOrbits;

	UPROPERTY()
	TArray<FNovaManeuver> Maneuvers;

	UPROPERTY()
	FNovaOrbit FinalOrbit;

	UPROPERTY()
	float TotalTransferDuration;

	UPROPERTY()
	float TotalDeltaV;
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
		return Trajectory == Other.Trajectory && SpacecraftIdentifier == Other.SpacecraftIdentifier;
	}

	UPROPERTY()
	FNovaTrajectory Trajectory;

	UPROPERTY()
	TArray<FGuid> SpacecraftIdentifier;
};

/** Trajectory database with fast array replication and fast lookup */
USTRUCT()
struct FNovaTrajectoryDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

public:
	/** Add a new trajectory to the database */
	void Add(const TSharedPtr<FNovaTrajectory>& Trajectory, const TArray<FGuid>& SpacecraftIdentifiers);

	/** Remove trajectories from the database */
	void Remove(const TArray<FGuid>& SpacecraftIdentifiers);

	/** Get a spacecraft */
	const FNovaTrajectory* Get(const FGuid& Identifier) const
	{
		const FNovaTrajectoryDatabaseEntry* const* Entry = Map.Find(Identifier);
		return Entry ? &(*Entry)->Trajectory : nullptr;
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaTrajectoryDatabaseEntry, FNovaTrajectoryDatabase>(
			Array, DeltaParms, *this);
	}

	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

protected:
	// Replicated state
	UPROPERTY()
	TArray<FNovaTrajectoryDatabaseEntry> Array;

	// Local state
	TMap<FGuid, FNovaTrajectoryDatabaseEntry*> Map;
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
