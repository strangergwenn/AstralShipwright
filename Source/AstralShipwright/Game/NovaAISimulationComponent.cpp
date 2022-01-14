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

		FNovaSpacecraft Spacecraft = SpacecraftDescription->Spacecraft;
		FNovaOrbit      Orbit      = FNovaOrbit(FNovaOrbitGeometry(DefaultPlanet, 400, 45), FNovaTime());
		Spacecraft.Name            = TEXT("Shitty Tug");

		GameState->UpdateSpacecraft(Spacecraft, &Orbit);
	}
}

void UNovaAISimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();

	// Iterate over all spacecraft
	if (PlayerLocation)
	{
		for (const TPair<FGuid, FNovaOrbitalLocation>& IdentifierAndLocation : OrbitalSimulation->GetAllSpacecraftLocations())
		{
			FGuid  Identifier         = IdentifierAndLocation.Key;
			double DistanceFromPlayer = IdentifierAndLocation.Value.GetDistanceTo(*PlayerLocation);

			if (PhysicalSpacecraftDatabase.Contains(Identifier))
			{
				// Spawn
				if (GetPhysicalSpacecraft(Identifier) == nullptr &&
					(Identifier == AlwaysLoadedSpacecraft || DistanceFromPlayer < SpacecraftSpawnDistanceKm))
				{
					ANovaSpacecraftPawn* NewSpacecraft = GetWorld()->SpawnActor<ANovaSpacecraftPawn>();
					NCHECK(NewSpacecraft);

					// TODO : probably need to pass something to the spacecraft

					NLOG("UNovaAISimulationComponent::TickComponent : spawning '%s'", *Identifier.ToString(EGuidFormats::Short));

					PhysicalSpacecraftDatabase.Add(Identifier, NewSpacecraft);
				}

				// De-spawn
				if (GetPhysicalSpacecraft(Identifier) != nullptr && !AlwaysLoadedSpacecraft.IsValid() &&
					DistanceFromPlayer > SpacecraftDespawnDistanceKm)
				{
					ANovaSpacecraftPawn** SpacecraftEntry = PhysicalSpacecraftDatabase.Find(Identifier);
					if (SpacecraftEntry)
					{
						NLOG("UNovaAISimulationComponent::TickComponent : removing '%s'", *Identifier.ToString(EGuidFormats::Short));

						(*SpacecraftEntry)->Destroy();
						PhysicalSpacecraftDatabase.Remove(Identifier);
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
