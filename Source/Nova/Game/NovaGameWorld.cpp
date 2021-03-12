// Nova project - Gwennaël Arbona

#include "NovaGameWorld.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaOrbitalSimulationComponent.h"
#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Spacecraft database
----------------------------------------------------*/

void FNovaSpacecraftDatabase::AddOrUpdate(const FNovaSpacecraft& Spacecraft)
{
	TPair<int32, FNovaSpacecraft*>* Entry = Map.Find(Spacecraft.Identifier);

	// Simple update
	if (Entry)
	{
		Array[Entry->Key] = Spacecraft;
		MarkItemDirty(Array[Entry->Key]);
	}

	// Full addition
	else
	{
		const int32 NewSpacecraftIndex = Array.Add(Spacecraft);
		MarkItemDirty(Array[NewSpacecraftIndex]);

		Map.Add(Spacecraft.Identifier, TPair<int32, FNovaSpacecraft*>(NewSpacecraftIndex, &Array[NewSpacecraftIndex]));
	}
}

void FNovaSpacecraftDatabase::Remove(const FNovaSpacecraft& Spacecraft)
{
	Array.RemoveSwap(Spacecraft);
	MarkArrayDirty();

	Map.Remove(Spacecraft.Identifier);
}

void FNovaSpacecraftDatabase::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	TArray<FGuid> KnownIdentifiers;

	// Database insertion and update
	int32 Index = 0;
	for (FNovaSpacecraft& Spacecraft : Array)
	{
		TPair<int32, FNovaSpacecraft*>* Entry = Map.Find(Spacecraft.Identifier);

		// Entry was found, update if necessary
		if (Entry)
		{
			if (*Entry->Value != Spacecraft)
			{
				Entry->Value = &Spacecraft;
			}
		}

		// Entry was not found, add it
		else
		{
			Map.Add(Spacecraft.Identifier, TPair<int32, FNovaSpacecraft*>(Index, &Spacecraft));
		}

		KnownIdentifiers.Add(Spacecraft.Identifier);
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

	NCHECK(Map.Num() == Array.Num());
}

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameWorld::ANovaGameWorld() : Super()
{
	// Setup simulation component
	OrbitalSimulationComponent = CreateDefaultSubobject<UNovaOrbitalSimulationComponent>(TEXT("OrbitalSimulationComponent"));

	// Settings
	bReplicates = true;
	SetReplicatingMovement(false);
	bAlwaysRelevant               = true;
	PrimaryActorTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaWorldSave
{
};

TSharedPtr<struct FNovaWorldSave> ANovaGameWorld::Save() const
{
	TSharedPtr<FNovaWorldSave> SaveData = MakeShared<FNovaWorldSave>();

	return SaveData;
}

void ANovaGameWorld::Load(TSharedPtr<struct FNovaWorldSave> SaveData)
{}

void ANovaGameWorld::SerializeJson(
	TSharedPtr<struct FNovaWorldSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction)
{}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaGameWorld::AddOrUpdateSpacecraft(const FNovaSpacecraft Identifier)
{
	SpacecraftDatabase.AddOrUpdate(Identifier);
}

FNovaSpacecraft* ANovaGameWorld::GetSpacecraft(FGuid Identifier)
{
	return SpacecraftDatabase.Get(Identifier);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaGameWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameWorld, SpacecraftDatabase);
};
