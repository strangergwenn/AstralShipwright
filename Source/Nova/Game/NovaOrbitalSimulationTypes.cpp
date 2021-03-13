// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationTypes.h"

/*----------------------------------------------------
    Simulation structures
----------------------------------------------------*/

float FNovaTrajectory::GetHighestAltitude() const
{
	float MaximumAltitude = 0;

	for (const FNovaOrbit& Orbit : TransferOrbits)
	{
		MaximumAltitude = FMath::Max(MaximumAltitude, Orbit.GetHighestAltitude());
	}

	return MaximumAltitude;
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
