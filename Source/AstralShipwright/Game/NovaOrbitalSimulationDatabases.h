// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaGameTypes.h"
#include "NovaOrbitalSimulationTypes.h"
#include "Spacecraft/NovaSpacecraft.h"

#include "NovaOrbitalSimulationDatabases.generated.h"

/*----------------------------------------------------
    Spacecraft database
----------------------------------------------------*/

/** Spacecraft database with fast array replication and fast lookup */
USTRUCT()
struct FNovaSpacecraftDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const FNovaSpacecraft& Spacecraft)
	{
		return Cache.Add(*this, Array, Spacecraft);
	}

	void Remove(const FGuid& Identifier)
	{
		Cache.Remove(*this, Array, Identifier);
	}

	const FNovaSpacecraft* Get(const FGuid& Identifier) const
	{
		return Cache.Get(Identifier, Array);
	}

	TArray<FNovaSpacecraft>& Get()
	{
		return Array;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaSpacecraft, FNovaSpacecraftDatabase>(Array, DeltaParms, *this);
	}

protected:
	UPROPERTY()
	TArray<FNovaSpacecraft> Array;

	TGuidCacheMap<FNovaSpacecraft> Cache;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaSpacecraftDatabase> : public TStructOpsTypeTraitsBase2<FNovaSpacecraftDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
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

	bool Add(const TArray<FGuid>& SpacecraftIdentifiers, const FNovaOrbit& Orbit)
	{
		NCHECK(Orbit.IsValid());

		FNovaOrbitDatabaseEntry TrajectoryData;
		TrajectoryData.Orbit       = Orbit;
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

	bool Add(const TArray<FGuid>& SpacecraftIdentifiers, const FNovaTrajectory& Trajectory)
	{
		NCHECK(Trajectory.IsValid());

		FNovaTrajectoryDatabaseEntry TrajectoryData;
		TrajectoryData.Trajectory  = Trajectory;
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

	int32 GetSpacecraftIndex(const FGuid& Identifier) const
	{
		const FNovaTrajectoryDatabaseEntry* Entry = Cache.Get(Identifier, Array);

		if (Entry)
		{
			for (int32 Index = 0; Index < Entry->Identifiers.Num(); Index++)
			{
				if (Entry->Identifiers[Index] == Identifier)
				{
					return Index;
				}
			}
		}

		return INDEX_NONE;
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
