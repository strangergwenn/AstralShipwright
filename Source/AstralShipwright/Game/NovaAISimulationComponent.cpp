// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaAISimulationComponent.h"

#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

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
}

/*----------------------------------------------------
    Internals high level
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

					GameState->SetTimeDilation(ENovaTimeDilation::Normal);
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
	// Get game state pointers
	UNovaAssetManager* AssetManager = GetOwner()->GetGameInstance<UNovaGameInstance>()->GetAssetManager();
	NCHECK(AssetManager);
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Iterate over the AI database
	for (TPair<FGuid, FNovaAISpacecraftState>& IdentifierAndSpacecraft : SpacecraftDatabase)
	{
		// Get more game state data
		FGuid                       Identifier      = IdentifierAndSpacecraft.Key;
		FNovaAISpacecraftState&     SpacecraftState = IdentifierAndSpacecraft.Value;
		const FNovaOrbitalLocation* SourceLocation  = OrbitalSimulation->GetSpacecraftLocation(Identifier);
		const FNovaOrbit*           SourceOrbit     = OrbitalSimulation->GetSpacecraftOrbit(Identifier);
		FNovaTime                   CurrentTime     = GameState->GetCurrentTime();

		// Get the physical spacecraft movement
		UNovaSpacecraftMovementComponent* SpacecraftMovement = nullptr;
		if (IsValid(SpacecraftState.PhysicalSpacecraft))
		{
			SpacecraftMovement = SpacecraftState.PhysicalSpacecraft->GetSpacecraftMovement();
		}

		// Issue new orders
		if (SpacecraftState.CurrentState == ENovaAISpacecraftState::Idle && SourceOrbit != nullptr)
		{
			NCHECK(SourceLocation != nullptr);

			// Pick a random destination that is not the nearest one
			TArray<const UNovaArea*> Areas           = AssetManager->GetAssets<UNovaArea>();
			auto                     AreaAndDistance = OrbitalSimulation->GetNearestAreaAndDistance(*SourceLocation);
			Areas.Remove(AreaAndDistance.Key);
			const UNovaArea* Area = Areas[FMath::RandHelper(Areas.Num())];
			NCHECK(Area);

			NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' now on trajectory toward '%s'",
				*Identifier.ToString(EGuidFormats::Short), *Area->Name.ToString());

			// Start the travel
			StartTrajectory(*SourceOrbit, OrbitalSimulation->GetAreaOrbit(Area), FNovaTime::FromSeconds(30), {Identifier});
			SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Trajectory);
		}

		// Wait for arrival
		else if (SpacecraftState.CurrentState == ENovaAISpacecraftState::Trajectory)
		{
			// Detect arrival
			const FNovaTrajectory* Trajectory = OrbitalSimulation->GetSpacecraftTrajectory(Identifier);
			if ((CurrentTime - SpacecraftState.CurrentStateStartTime > FNovaTime::FromMinutes(5)) &&
				(Trajectory == nullptr || Trajectory->GetArrivalTime() < CurrentTime))
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' arriving at station", *Identifier.ToString(EGuidFormats::Short));

				SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Station);
			}

			// Align to maneuvers
			if (IsValid(SpacecraftMovement) && SpacecraftMovement->IsIdle() && !SpacecraftMovement->IsAlignedToManeuver())
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' aligning for maneuver",
					*Identifier.ToString(EGuidFormats::Short));

				SpacecraftMovement->AlignToManeuver();
			}
		}

		// Stay at the station for some time
		else if (SpacecraftState.CurrentState == ENovaAISpacecraftState::Station)
		{
			// Detect enough time spent
			if (CurrentTime - SpacecraftState.CurrentStateStartTime > FNovaTime::FromMinutes(5))
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' undocking", *Identifier.ToString(EGuidFormats::Short));

				SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Undocking);
			}

			// Dock if we're not already docked or undocking
			else if (IsValid(SpacecraftMovement) && SpacecraftMovement->IsIdle() && !SpacecraftMovement->IsDockingUndocking() &&
					 !SpacecraftMovement->IsDocked())
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' docking", *Identifier.ToString(EGuidFormats::Short));

				SpacecraftMovement->Dock();
			}
		}

		// Check for complete undocking
		else if (SpacecraftState.CurrentState == ENovaAISpacecraftState::Undocking)
		{
			// Detect no physical ship OR idle and undocked
			if (!IsValid(SpacecraftMovement) || (SpacecraftMovement->IsIdle() && !SpacecraftMovement->IsDocked()))
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' going idle", *Identifier.ToString(EGuidFormats::Short));

				SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Idle);
			}

			// Undock if we're not already undocked
			else if (IsValid(SpacecraftMovement) && SpacecraftMovement->IsIdle() && !SpacecraftMovement->IsDockingUndocking() &&
					 SpacecraftMovement->IsDocked())
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' undocking", *Identifier.ToString(EGuidFormats::Short));

				SpacecraftMovement->Undock();
			}
		}
	}
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

void UNovaAISimulationComponent::SetSpacecraftState(FNovaAISpacecraftState& State, ENovaAISpacecraftState NewState)
{
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	FNovaTime CurrentTime = GameState->GetCurrentTime();

	State.CurrentState          = NewState;
	State.CurrentStateStartTime = CurrentTime;

	// GameState->SetTimeDilation(ENovaTimeDilation::Normal);
}

void UNovaAISimulationComponent::StartTrajectory(
	const FNovaOrbit& SourceOrbit, const FNovaOrbit& DestinationOrbit, FNovaTime DeltaTime, const TArray<FGuid>& Spacecraft)
{
	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Compute trajectory candidates
	TArray<FNovaTrajectory>   Candidates;
	FNovaTrajectoryParameters Parameters = OrbitalSimulation->PrepareTrajectory(SourceOrbit, DestinationOrbit, DeltaTime, Spacecraft);
	for (float Altitude = 300; Altitude <= 1500; Altitude += 300)
	{
		FNovaTrajectory NewTrajectory = OrbitalSimulation->ComputeTrajectory(Parameters, Altitude);
		if (NewTrajectory.IsValid() && NewTrajectory.TotalTravelDuration.AsDays() < 15)
		{
			Candidates.Add(NewTrajectory);
		}
	}

	NCHECK(Candidates.Num() > 0);

	// Sort trajectories
	Candidates.Sort(
		[](const FNovaTrajectory& A, const FNovaTrajectory& B)
		{
			return A.TotalTravelDuration < B.TotalTravelDuration && A.TotalDeltaV < B.TotalDeltaV;
		});

	// Start trajectory
	OrbitalSimulation->CommitTrajectory(Spacecraft, Candidates[0]);
}

#undef LOCTEXT_NAMESPACE
