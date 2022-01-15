// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaAISimulationComponent.h"

#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Spacecraft/NovaSpacecraftPawn.h"

#include "System/NovaAssetManager.h"
#include "System/NovaGameInstance.h"

#include "Nova.h"

#define LOCTEXT_NAMESPACE "UNovaAISimulationComponent"

// Definitions
static constexpr int32 SpacecraftSpawnDistanceKm   = 500;
static constexpr int32 SpacecraftDespawnDistanceKm = 600;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaAISimulationComponent::UNovaAISimulationComponent() : Super()
{
	// Settings
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void UNovaAISimulationComponent::Initialize()
{
	NLOG("UNovaAISimulationComponent::Initialize");

	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		// Get game state pointers
		UNovaAssetManager* AssetManager = GetOwner()->GetGameInstance<UNovaGameInstance>()->GetAssetManager();
		NCHECK(AssetManager);
		ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
		NCHECK(GameState);

		// Spawn spacecraft
		for (const UNovaAISpacecraftDescription* SpacecraftDescription : AssetManager->GetAssets<UNovaAISpacecraftDescription>())
		{
			const class UNovaCelestialBody* DefaultPlanet =
				AssetManager->GetAsset<UNovaCelestialBody>(FGuid("{0619238A-4DD1-E28B-5F86-A49734CEF648}"));

			FNovaOrbit Orbit = FNovaOrbit(FNovaOrbitGeometry(DefaultPlanet, 400, 45), FNovaTime());

			// Create the spacecraft
			FNovaSpacecraft Spacecraft = SpacecraftDescription->Spacecraft;
			Spacecraft.Name            = TEXT("Shitty Tug");
			Spacecraft.SpacecraftClass = SpacecraftDescription;

			// Register the spacecraft
			SpacecraftDatabase.Add(Spacecraft.Identifier, FNovaAISpacecraftState());
			GameState->UpdateSpacecraft(Spacecraft, &Orbit);
		}
	}
}

void UNovaAISimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Run processes
	ProcessSpawning();
	ProcessNavigation();
	ProcessPhysicalMovement();
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaAISimulationComponent::ProcessSpawning()
{
	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();

	// Iterate over all spacecraft locations
	if (PlayerLocation)
	{
		for (const TPair<FGuid, FNovaOrbitalLocation>& IdentifierAndLocation : OrbitalSimulation->GetAllSpacecraftLocations())
		{
			FGuid                         Identifier         = IdentifierAndLocation.Key;
			double                        DistanceFromPlayer = IdentifierAndLocation.Value.GetDistanceTo(*PlayerLocation);
			const FNovaAISpacecraftState* SpacecraftStatePtr = SpacecraftDatabase.Find(Identifier);

			if (SpacecraftStatePtr)
			{
				// Spawn
				if (!IsValid(SpacecraftStatePtr->PhysicalSpacecraft) &&
					(Identifier == AlwaysLoadedSpacecraft || DistanceFromPlayer < SpacecraftSpawnDistanceKm))
				{
					ANovaSpacecraftPawn* NewSpacecraft = GetWorld()->SpawnActor<ANovaSpacecraftPawn>();
					NCHECK(NewSpacecraft);
					NewSpacecraft->SetSpacecraftIdentifier(Identifier);

					NLOG("UNovaAISimulationComponent::ProcessSpawning : spawning '%s'", *Identifier.ToString(EGuidFormats::Short));

					SpacecraftDatabase[Identifier].PhysicalSpacecraft = NewSpacecraft;
				}

				// De-spawn
				if (IsValid(SpacecraftStatePtr->PhysicalSpacecraft) && !AlwaysLoadedSpacecraft.IsValid() &&
					DistanceFromPlayer > SpacecraftDespawnDistanceKm)
				{
					NLOG("UNovaAISimulationComponent::ProcessSpawning : removing '%s'", *Identifier.ToString(EGuidFormats::Short));

					SpacecraftStatePtr->PhysicalSpacecraft->Destroy();
					SpacecraftDatabase[Identifier].PhysicalSpacecraft = nullptr;
				}
			}
		}
	}
}

void UNovaAISimulationComponent::ProcessNavigation()
{
	// Iterate over the AI database
	for (TPair<FGuid, FNovaAISpacecraftState>& IdentifierAndSpacecraft : SpacecraftDatabase)
	{
		FGuid                   Identifier      = IdentifierAndSpacecraft.Key;
		FNovaAISpacecraftState& SpacecraftState = IdentifierAndSpacecraft.Value;
	}
}

void UNovaAISimulationComponent::ProcessPhysicalMovement()
{
	// Iterate over all physical AI spacecraft
	for (TPair<FGuid, FNovaAISpacecraftState>& IdentifierAndSpacecraft : SpacecraftDatabase)
	{
		FGuid                   Identifier      = IdentifierAndSpacecraft.Key;
		FNovaAISpacecraftState& SpacecraftState = IdentifierAndSpacecraft.Value;

		if (IsValid(SpacecraftState.PhysicalSpacecraft))
		{
		}
	}
}

#undef LOCTEXT_NAMESPACE
