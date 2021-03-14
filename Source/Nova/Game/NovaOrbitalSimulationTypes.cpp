// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationTypes.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

float FNovaTrajectory::GetHighestAltitude() const
{
	NCHECK(IsValid());

	float MaximumAltitude = 0;

	for (const FNovaOrbit& Orbit : TransferOrbits)
	{
		MaximumAltitude = FMath::Max(MaximumAltitude, Orbit.GetHighestAltitude());
	}

	return MaximumAltitude;
}

FNovaTimedOrbit FNovaTrajectory::GetFinalOrbit() const
{
	NCHECK(IsValid());

	// Assume the final maneuver is a circularization burn
	FNovaOrbit FinalOrbit    = TransferOrbits[TransferOrbits.Num() - 1];
	FinalOrbit.StartAltitude = FinalOrbit.OppositeAltitude;
	FinalOrbit.StartPhase    = FinalOrbit.EndPhase;

	// Assume maneuvers are of zero time
	const FNovaManeuver& FinalManeuver = Maneuvers[Maneuvers.Num() - 1];
	const float&         InsertionTime = FinalManeuver.Time;

	return FNovaTimedOrbit(FinalOrbit, InsertionTime);
}

/*----------------------------------------------------
    Orbit database
----------------------------------------------------*/

void FNovaOrbitDatabase::Add(const TSharedPtr<FNovaTimedOrbit>& Orbit, const TArray<FGuid>& SpacecraftIdentifiers)
{
	FNovaOrbitDatabaseEntry TrajectoryData;
	TrajectoryData.Orbit                = *Orbit.Get();
	TrajectoryData.SpacecraftIdentifier = SpacecraftIdentifiers;

	const int32 NewTrajectoryIndex = Array.Add(TrajectoryData);
	MarkItemDirty(Array[NewTrajectoryIndex]);

	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		Map.Add(Identifier, &Array[NewTrajectoryIndex]);
	}
}

void FNovaOrbitDatabase::Remove(const TArray<FGuid>& SpacecraftIdentifiers)
{
	bool IsDirty = false;

	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		FNovaOrbitDatabaseEntry** Entry = Map.Find(Identifier);
		if (Entry)
		{
			Array.RemoveSwap(**Entry);
			IsDirty = true;
		}

		Map.Remove(Identifier);
	}

	if (IsDirty)
	{
		MarkArrayDirty();
	}
}

void FNovaOrbitDatabase::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	TArray<FGuid> KnownIdentifiers;

	// Database insertion and update
	int32 Index = 0;
	for (FNovaOrbitDatabaseEntry& TrajectoryData : Array)
	{
		for (const FGuid& Identifier : TrajectoryData.SpacecraftIdentifier)
		{
			FNovaOrbitDatabaseEntry** Entry = Map.Find(Identifier);

			// Entry was found, update if necessary
			if (Entry)
			{
				*Entry = &TrajectoryData;
			}

			// Entry was not found, add it
			else
			{
				Map.Add(Identifier, &TrajectoryData);
			}

			KnownIdentifiers.Add(Identifier);
			Index++;
		}
	}

	// Prune unused entries
	for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
	{
		if (!KnownIdentifiers.Contains(Iterator.Key()))
		{
			Iterator.RemoveCurrent();
		}
	}
}

/*----------------------------------------------------
    Trajectory database
----------------------------------------------------*/

void FNovaTrajectoryDatabase::Add(const TSharedPtr<FNovaTrajectory>& Trajectory, const TArray<FGuid>& SpacecraftIdentifiers)
{
	FNovaTrajectoryDatabaseEntry TrajectoryData;
	TrajectoryData.Trajectory           = *Trajectory.Get();
	TrajectoryData.SpacecraftIdentifier = SpacecraftIdentifiers;

	const int32 NewTrajectoryIndex = Array.Add(TrajectoryData);
	MarkItemDirty(Array[NewTrajectoryIndex]);

	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		Map.Add(Identifier, &Array[NewTrajectoryIndex]);
	}
}

void FNovaTrajectoryDatabase::Remove(const TArray<FGuid>& SpacecraftIdentifiers)
{
	bool IsDirty = false;

	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		FNovaTrajectoryDatabaseEntry** Entry = Map.Find(Identifier);
		if (Entry)
		{
			Array.RemoveSwap(**Entry);
			IsDirty = true;
		}

		Map.Remove(Identifier);
	}

	if (IsDirty)
	{
		MarkArrayDirty();
	}
}

void FNovaTrajectoryDatabase::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	TArray<FGuid> KnownIdentifiers;

	// Database insertion and update
	int32 Index = 0;
	for (FNovaTrajectoryDatabaseEntry& TrajectoryData : Array)
	{
		for (const FGuid& Identifier : TrajectoryData.SpacecraftIdentifier)
		{
			FNovaTrajectoryDatabaseEntry** Entry = Map.Find(Identifier);

			// Entry was found, update if necessary
			if (Entry)
			{
				*Entry = &TrajectoryData;
			}

			// Entry was not found, add it
			else
			{
				Map.Add(Identifier, &TrajectoryData);
			}

			KnownIdentifiers.Add(Identifier);
			Index++;
		}
	}

	// Prune unused entries
	for (auto Iterator = Map.CreateIterator(); Iterator; ++Iterator)
	{
		if (!KnownIdentifiers.Contains(Iterator.Key()))
		{
			Iterator.RemoveCurrent();
		}
	}
}
