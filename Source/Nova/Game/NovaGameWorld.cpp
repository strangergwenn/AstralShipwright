// Nova project - Gwennaël Arbona

#include "NovaGameWorld.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaOrbitalSimulationComponent.h"
#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

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
	TSharedPtr<FNovaWorldSave> SaveData = MakeShareable(new FNovaWorldSave);

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

void ANovaGameWorld::AddAISpacecraft(const FNovaSpacecraft Spacecraft)
{
	AISpacecraft.Add(Spacecraft);
	UpdateDatabase();
}

void ANovaGameWorld::AddPlayerSpacecraft(const FNovaSpacecraft Spacecraft)
{
	PlayerSpacecraft.Add(Spacecraft);
	UpdateDatabase();
}

TSharedPtr<FNovaSpacecraft> ANovaGameWorld::GetSpacecraft(FGuid Identifier)
{
	FNovaSpacecraftDatabaseEntry* Entry = SpacecraftDatabase.Find(Identifier);
	return Entry ? Entry->Spacecraft : nullptr;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaGameWorld::OnSpacecraftReplicated()
{
	// TODO : use custom net serializer to make all of that a non issue

	UpdateDatabase();
}

void ANovaGameWorld::UpdateDatabase()
{
	NLOG("ANovaGameWorld::UpdateDatabase");

	TArray<FGuid> KnownIdentifiers;

	// Database insertion and update
	auto UpdateDatabaseFromArray = [this, &KnownIdentifiers](const TArray<FNovaSpacecraft>& Array, bool IsPlayer)
	{
		for (const FNovaSpacecraft& Spacecraft : Array)
		{
			FNovaSpacecraftDatabaseEntry* Entry = SpacecraftDatabase.Find(Spacecraft.Identifier);

			// Entry was found, update if necessary
			if (Entry)
			{
				if (*Entry->Spacecraft.Get() != Spacecraft)
				{
					Entry->Spacecraft = Spacecraft.GetSharedCopy();
				}
			}

			// Entry was not found, add it
			else
			{
				FNovaSpacecraftDatabaseEntry NewEntry;
				NewEntry.IsPlayer   = IsPlayer;
				NewEntry.Spacecraft = Spacecraft.GetSharedCopy();
				SpacecraftDatabase.Add(Spacecraft.Identifier, NewEntry);
			}

			KnownIdentifiers.Add(Spacecraft.Identifier);
		}
	};

	// Database garbage collection
	auto PruneDatabase = [this, &KnownIdentifiers]()
	{
		for (auto Iterator = SpacecraftDatabase.CreateIterator(); Iterator; ++Iterator)
		{
			if (!KnownIdentifiers.Contains(Iterator.Key()))
			{
				Iterator.RemoveCurrent();
			}
		}
	};

	// Run the insertion and update process, then collect the garbage
	UpdateDatabaseFromArray(AISpacecraft, false);
	UpdateDatabaseFromArray(PlayerSpacecraft, true);
	PruneDatabase();
}

void ANovaGameWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameWorld, AISpacecraft);
	DOREPLIFETIME(ANovaGameWorld, PlayerSpacecraft);
};