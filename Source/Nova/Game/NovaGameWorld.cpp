// Nova project - Gwennaël Arbona

#include "NovaGameWorld.h"

#include "NovaArea.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaGameState.h"
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

ANovaGameWorld* ANovaGameWorld::Get(const UObject* Outer)
{
	ANovaGameState* GameState = Outer->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);

	return GameWorld;
}

void ANovaGameWorld::UpdateSpacecraft(const FNovaSpacecraft Spacecraft, bool IsPlayerSpacecraft)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	NLOG("ANovaGameWorld::UpdateSpacecraft");

	bool IsNew = SpacecraftDatabase.AddOrUpdate(Spacecraft);

	if (IsNew)
	{
		// Attempt orbit merging for player spacecraft joining the game
		bool HasMergedOrbits = false;
		if (IsPlayerSpacecraft)
		{
			ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
			NCHECK(GameState);
			const FNovaOrbit* CurrentOrbit = OrbitalSimulationComponent->GetPlayerOrbit();

			if (CurrentOrbit)
			{
				OrbitalSimulationComponent->MergeOrbit(GameState->GetPlayerSpacecraftIdentifiers(), MakeShared<FNovaOrbit>(*CurrentOrbit));
				HasMergedOrbits = true;
			}
		}

		// Load a default
		if (!HasMergedOrbits)
		{
			// TODO : should first look into deserialized save data, and then if nothing, fetch the default location from game mode

			const class UNovaArea* StationA =
				GetGameInstance<UNovaGameInstance>()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));

			OrbitalSimulationComponent->SetOrbit({Spacecraft.Identifier}, OrbitalSimulationComponent->GetAreaOrbit(StationA));
		}
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaGameWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameWorld, SpacecraftDatabase);
};
