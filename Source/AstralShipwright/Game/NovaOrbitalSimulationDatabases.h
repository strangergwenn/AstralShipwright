// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaGameTypes.h"
#include "NovaOrbitalSimulationTypes.h"
#include "Spacecraft/NovaSpacecraft.h"

#include "NovaOrbitalSimulationDatabases.generated.h"

/*----------------------------------------------------
    Cache maps for fast lookup net serialized arrays
----------------------------------------------------*/

/** Map of FGuid -> T* that mirrors a TArray<T>, with FGuid == T.Identifier */
template <typename T>
struct TGuidCacheMap
{
	/** Add or update ArrayItem to the array Array held by structure Serializer */
	bool Add(FFastArraySerializer& Serializer, TArray<T>& Array, const T& ArrayItem)
	{
		bool NewEntry = false;

		TPair<int32, const T*>* Entry = Map.Find(ArrayItem.Identifier);

		// Simple update
		if (Entry)
		{
			Array[Entry->Key] = ArrayItem;
			Serializer.MarkItemDirty(Array[Entry->Key]);
		}

		// Full addition
		else
		{
			const int32 NewIndex = Array.Add(ArrayItem);
			Serializer.MarkItemDirty(Array[NewIndex]);

			NewEntry = true;
		}

		Update(Array);

		return NewEntry;
	}

	/** Remove the item associated with Identifier from the array Array held by structure Serializer */
	void Remove(FFastArraySerializer& Serializer, TArray<T>& Array, const FGuid& Identifier)
	{
		TPair<int32, const T*>* Entry = Map.Find(Identifier);

		if (Entry)
		{
			Array.RemoveAt(Entry->Key);
			Serializer.MarkArrayDirty();
		}

		Update(Array);
	}

	/** Get an item by identifier */
	const T* Get(const FGuid& Identifier, const TArray<T>& Array) const
	{
		const TPair<int32, const T*>* Entry = Map.Find(Identifier);

		if (Entry)
		{
			NCHECK(Entry->Key >= 0 && Entry->Key < Array.Num());
			NCHECK(Entry->Value == &Array[Entry->Key]);
		}

		return Entry ? Entry->Value : nullptr;
	}

	/** Update the map from the array Array */
	void Update(const TArray<T>& Array)
	{
		TArray<FGuid> KnownIdentifiers;

		// Database insertion and update
		int32 Index = 0;
		for (const T& ArrayItem : Array)
		{
			TPair<int32, const T*>* Entry = Map.Find(ArrayItem.Identifier);

			// Entry was found, update if necessary
			if (Entry)
			{
				if (*Entry->Value != ArrayItem)
				{
					Entry->Value = &ArrayItem;
				}
			}

			// Entry was not found, add it
			else
			{
				Map.Add(ArrayItem.Identifier, TPair<int32, const T*>(Index, &ArrayItem));
			}

			KnownIdentifiers.Add(ArrayItem.Identifier);
			Index++;
		}

		// Prune unused entries
		for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
		{
			if (!KnownIdentifiers.Contains(Iterator.Key()))
			{
				Iterator.RemoveCurrent();
			}
		}

		for (auto& IdentifierAndEntry : Map)
		{
			NCHECK(IdentifierAndEntry.Value.Key >= 0 && IdentifierAndEntry.Value.Key < Array.Num());
			NCHECK(IdentifierAndEntry.Value.Value == &Array[IdentifierAndEntry.Value.Key]);
		}
	}

	TMap<FGuid, TPair<int32, const T*>> Map;
};

/** Map of multiple FGuid -> T* that mirrors a TArray<T>, with TArray<FGuid> == T.Identifiers */
template <typename T>
struct TMultiGuidCacheMap
{
	/** Add or update ArrayItem to the array Array held by structure Serializer */
	bool Add(FFastArraySerializer& Serializer, TArray<T>& Array, const T& ArrayItem)
	{
		bool NewEntry = false;

		// Find the existing entry
		int32 ExistingItemIndex = INDEX_NONE;
		for (const FGuid& Identifier : ArrayItem.Identifiers)
		{
			TPair<int32, const T*>* Entry = Map.Find(Identifier);
			if (Entry)
			{
				NCHECK(ExistingItemIndex == INDEX_NONE || Entry->Key == ExistingItemIndex);
				ExistingItemIndex = Entry->Key;
			}
		}

		// Simple update
		if (ExistingItemIndex != INDEX_NONE)
		{
			Array[ExistingItemIndex] = ArrayItem;
			Serializer.MarkItemDirty(Array[ExistingItemIndex]);
		}

		// Full addition
		else
		{
			const int32 NewIndex = Array.Add(ArrayItem);
			Serializer.MarkItemDirty(Array[NewIndex]);

			NewEntry = true;
		}

		Update(Array);

		return NewEntry;
	}

	/** Remove the item associated with Identifiers from the array Array held by structure Serializer
	 * All Identifiers are assumed to point to the same ItemArray
	 */
	void Remove(FFastArraySerializer& Serializer, TArray<T>& Array, const TArray<FGuid>& Identifiers)
	{
		bool IsDirty = false;

		// Find the existing entry
		int32 ExistingItemIndex = INDEX_NONE;
		for (const FGuid& Identifier : Identifiers)
		{
			TPair<int32, const T*>* Entry = Map.Find(Identifier);
			if (Entry)
			{
				NCHECK(ExistingItemIndex == INDEX_NONE || Entry->Key == ExistingItemIndex);
				ExistingItemIndex = Entry->Key;
			}
		}

		// Delete the entry
		if (ExistingItemIndex != INDEX_NONE)
		{
			Array.RemoveAt(ExistingItemIndex);
			Serializer.MarkArrayDirty();
		}

		Update(Array);
	}

	/** Get an item by identifier */
	const T* Get(const FGuid& Identifier, const TArray<T>& Array) const
	{
		const TPair<int32, const T*>* Entry = Map.Find(Identifier);

		if (Entry)
		{
			NCHECK(Entry->Key >= 0 && Entry->Key < Array.Num());
			NCHECK(Entry->Value == &Array[Entry->Key]);
		}

		return Entry ? Entry->Value : nullptr;
	}

	/** Update the map from the array Array */
	void Update(const TArray<T>& Array)
	{
		TArray<FGuid> KnownIdentifiers;

		// Database insertion and update
		int32 Index = 0;
		for (const T& ArrayItem : Array)
		{
			for (const FGuid& Identifier : ArrayItem.Identifiers)
			{
				TPair<int32, const T*>* Entry = Map.Find(Identifier);

				// Entry was found, update if necessary
				if (Entry)
				{
					Entry->Value = &ArrayItem;
				}

				// Entry was not found, add it
				else
				{
					Map.Add(Identifier, TPair<int32, const T*>(Index, &ArrayItem));
				}

				KnownIdentifiers.Add(Identifier);
			}

			Index++;
		}

		// Prune unused entries
		for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
		{
			if (!KnownIdentifiers.Contains(Iterator.Key()))
			{
				Iterator.RemoveCurrent();
			}
		}

		for (auto& IdentifierAndEntry : Map)
		{
			NCHECK(IdentifierAndEntry.Value.Key >= 0 && IdentifierAndEntry.Value.Key < Array.Num());
			NCHECK(IdentifierAndEntry.Value.Value == &Array[IdentifierAndEntry.Value.Key]);
		}
	}

	TMap<FGuid, TPair<int32, const T*>> Map;
};

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
