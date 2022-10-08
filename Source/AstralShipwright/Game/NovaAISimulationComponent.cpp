// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaAISimulationComponent.h"

#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"

#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"

#define LOCTEXT_NAMESPACE "UNovaAISimulationComponent"

/*----------------------------------------------------
    Definitions
----------------------------------------------------*/

// Procedural generation
static constexpr int32  InitialTechnicalSpacecraftCount = 42;
static constexpr double InitialMinAltitude              = 400.0;
static constexpr double InitialMaxAltitude              = 1000.0;

// Spawning
static constexpr int32 SpacecraftSpawnDistanceKm   = 100;
static constexpr int32 SpacecraftDespawnDistanceKm = 200;

// Behavior timings in minutes
static constexpr int32 TrajectoryMinDuration = 5;
static constexpr int32 StationWaitTime       = 5;
static constexpr int32 StationPatrolTimeout  = 10;

// Patrol
static constexpr double PatrolMinAltitude = 400.0;
static constexpr double PatrolMaxAltitude = 1000.0;

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaAISimulationComponent::UNovaAISimulationComponent() : Super(), PatrolRandomStream(0)
{
	// Technical ship names
	TechnicalNamePrefixes = {TEXT("Analog"), TEXT("Broken"), TEXT("Clockwork"), TEXT("Drab"), TEXT("Electric"), TEXT("Flying"),
		TEXT("Greasy"), TEXT("Happy"), TEXT("Inert"), TEXT("Jittery"), TEXT("Leaky"), TEXT("Moisty"), TEXT("Noisy"), TEXT("Old"),
		TEXT("Putrid"), TEXT("Rusty"), TEXT("Sleepy"), TEXT("Troubled"), TEXT("Ugly"), TEXT("Valiant"), TEXT("Wild"), TEXT("Zealous")};
	TechnicalNameSuffixes = {TEXT("Anvil"), TEXT("Brick"), TEXT("Chariot"), TEXT("Driller"), TEXT("Explorer"), TEXT("Farmer"), TEXT("Gear"),
		TEXT("Hammer"), TEXT("Ingot"), TEXT("Joker"), TEXT("Knocker"), TEXT("Laborer"), TEXT("Miner"), TEXT("Nail"), TEXT("Operator"),
		TEXT("Prospector"), TEXT("Quantum"), TEXT("Roller"), TEXT("Shovel"), TEXT("Tug"), TEXT("Unit"), TEXT("Wedge"), TEXT("Whale"),
		TEXT("Yield"), TEXT("Zero")};

	// Settings
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

FNovaAIStateSave UNovaAISimulationComponent::Save() const
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	FNovaAIStateSave SaveData;

	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Iterate over the AI database
	for (const TPair<FGuid, FNovaAISpacecraftState>& IdentifierAndSpacecraft : SpacecraftDatabase)
	{
		FNovaAISpacecraftStateSave SpacecraftSaveData;

		FGuid                         Identifier      = IdentifierAndSpacecraft.Key;
		const FNovaAISpacecraftState& SpacecraftState = IdentifierAndSpacecraft.Value;
		const FNovaOrbit*             Orbit           = OrbitalSimulation->GetSpacecraftOrbit(Identifier);
		const FNovaTrajectory*        Trajectory      = OrbitalSimulation->GetSpacecraftTrajectory(Identifier);

		// Spacecraft
		const FNovaSpacecraft* Spacecraft = GameState->GetSpacecraft(Identifier);
		NCHECK(Spacecraft);
		SpacecraftSaveData.SpacecraftIdentifier = Identifier;
		SpacecraftSaveData.SpacecraftClass      = Spacecraft->SpacecraftClass;
		SpacecraftSaveData.SpacecraftName       = Spacecraft->Name;

		// State
		SpacecraftSaveData.TargetArea            = SpacecraftState.TargetArea;
		SpacecraftSaveData.CurrentState          = SpacecraftState.CurrentState;
		SpacecraftSaveData.CurrentStateStartTime = SpacecraftState.CurrentStateStartTime;

		// Trajectory & orbit
		if (Trajectory)
		{
			SpacecraftSaveData.Trajectory = *Trajectory;
		}
		else if (Orbit)
		{
			SpacecraftSaveData.Orbit = *Orbit;
		}

		// Sanity checks
		NCHECK(SpacecraftSaveData.SpacecraftIdentifier.IsValid());
		NCHECK(SpacecraftSaveData.SpacecraftClass != nullptr);
		NCHECK(SpacecraftSaveData.SpacecraftName.Len() > 0);

		SaveData.SpacecraftStates.Add(SpacecraftSaveData);
	}

	// Save patrol state
	SaveData.PatrolRandomStreamSeed = PatrolRandomStream.GetCurrentSeed();

	return SaveData;
}

void UNovaAISimulationComponent::Load(const FNovaAIStateSave& SaveData)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaAISimulationComponent::Load");

	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Load actual data
	if (SaveData.SpacecraftStates.Num() > 0)
	{
		// Iterate over the save data
		for (const FNovaAISpacecraftStateSave& SpacecraftSaveData : SaveData.SpacecraftStates)
		{
			FNovaAISpacecraftState SpacecraftState;

			// Sanity checks
			NCHECK(SpacecraftSaveData.SpacecraftIdentifier.IsValid());
			NCHECK(SpacecraftSaveData.SpacecraftClass != nullptr);
			NCHECK(SpacecraftSaveData.SpacecraftName.Len() > 0);

			// Spacecraft
			FNovaSpacecraft Spacecraft = SpacecraftSaveData.SpacecraftClass->Spacecraft;
			Spacecraft.Name            = SpacecraftSaveData.SpacecraftName;
			Spacecraft.SpacecraftClass = SpacecraftSaveData.SpacecraftClass;
			Spacecraft.Identifier      = SpacecraftSaveData.SpacecraftIdentifier;

			// Common
			SpacecraftState.TargetArea            = SpacecraftSaveData.TargetArea;
			SpacecraftState.CurrentState          = SpacecraftSaveData.CurrentState;
			SpacecraftState.CurrentStateStartTime = SpacecraftSaveData.CurrentStateStartTime;

			// Register the spacecraft
			SpacecraftDatabase.Add(Spacecraft.Identifier, SpacecraftState);
			GameState->UpdateSpacecraft(Spacecraft, &SpacecraftSaveData.Orbit);
			if (SpacecraftSaveData.Trajectory.IsValid())
			{
				OrbitalSimulation->CommitTrajectory({Spacecraft.Identifier}, SpacecraftSaveData.Trajectory);
			}
		}

		// Load patrol state
		PatrolRandomStream = FRandomStream(SaveData.PatrolRandomStreamSeed);
	}

	// New game
	else
	{
		CreateGame();
	}
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void UNovaAISimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Run local processes
	ProcessSpawning();

	// Run server processes
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		ProcessQuotas();
		ProcessNavigation();
	}
}

/*----------------------------------------------------
    Internals high level
----------------------------------------------------*/

void UNovaAISimulationComponent::ProcessQuotas()
{
	AreasQuotas.Empty();

	// Iterate over the AI database
	for (const TPair<FGuid, FNovaAISpacecraftState>& IdentifierAndSpacecraft : SpacecraftDatabase)
	{
		const FNovaAISpacecraftState& SpacecraftState = IdentifierAndSpacecraft.Value;

		if (SpacecraftState.TargetArea)
		{
			int32* QuotaPtr = AreasQuotas.Find(SpacecraftState.TargetArea);
			if (QuotaPtr)
			{
				(*QuotaPtr)++;
			}
			else
			{
				AreasQuotas.Add(SpacecraftState.TargetArea, 1);
			}
		}
	}
}

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
		for (const TPair<FGuid, FNovaOrbitalLocation>& IdentifierAndLocation : OrbitalSimulation->GetSpacecraftLocations())
		{
			FGuid                         Identifier         = IdentifierAndLocation.Key;
			double                        DistanceFromPlayer = IdentifierAndLocation.Value.GetDistanceTo(*PlayerLocation);
			const FNovaAISpacecraftState* SpacecraftStatePtr = SpacecraftDatabase.Find(Identifier);

			if (SpacecraftStatePtr)
			{
				// Ships are considered private when they're at any unloaded station
				bool StationPrivate = SpacecraftStatePtr->CurrentState == ENovaAISpacecraftState::Station &&
				                      IsValid(SpacecraftStatePtr->TargetArea) &&
				                      SpacecraftStatePtr->TargetArea->LevelName != GameState->GetCurrentLevelName();

				// Spawn
				if (!IsValid(SpacecraftStatePtr->PhysicalSpacecraft) && !StationPrivate &&
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
					(DistanceFromPlayer > SpacecraftDespawnDistanceKm || StationPrivate))
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
			const UNovaArea* TargetArea = FindArea(SourceLocation);

			// Station
			if (TargetArea)
			{
				// Pick the area
				SpacecraftState.TargetArea = TargetArea;
				NCHECK(SpacecraftState.TargetArea);

				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' now on trajectory toward '%s'",
					*Identifier.ToString(EGuidFormats::Short), *SpacecraftState.TargetArea->Name.ToString());

				// Start the travel
				StartTrajectory(
					*SourceOrbit, OrbitalSimulation->GetAreaOrbit(SpacecraftState.TargetArea), FNovaTime::FromSeconds(30), {Identifier});
				SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Trajectory);
			}

			// Patrol
			else
			{
				SpacecraftState.TargetArea = nullptr;

				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' now on patrol trajectory",
					*Identifier.ToString(EGuidFormats::Short));

				// Start the travel
				FNovaOrbit DestinationOrbit;
				FindPatrolOrbit(DestinationOrbit);
				StartTrajectory(*SourceOrbit, DestinationOrbit, FNovaTime::FromSeconds(30), {Identifier},
					1.25 * DestinationOrbit.Geometry.StartAltitude);
				SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Trajectory);
			}

			// Run quotas again
			ProcessQuotas();
		}

		// Wait for arrival
		else if (SpacecraftState.CurrentState == ENovaAISpacecraftState::Trajectory)
		{
			// Detect arrival
			const FNovaTrajectory* Trajectory = OrbitalSimulation->GetSpacecraftTrajectory(Identifier);
			if ((CurrentTime - SpacecraftState.CurrentStateStartTime > FNovaTime::FromMinutes(TrajectoryMinDuration)) &&
				(Trajectory == nullptr || Trajectory->GetArrivalTime() < CurrentTime))
			{
				if (SpacecraftState.TargetArea)
				{
					NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' arriving at station",
						*Identifier.ToString(EGuidFormats::Short));

					SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Station);
				}
				else
				{
					NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' arriving at patrol location",
						*Identifier.ToString(EGuidFormats::Short));

					SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Idle);
				}
			}
		}

		// Stay at the station for some time
		else if (SpacecraftState.CurrentState == ENovaAISpacecraftState::Station)
		{
			const UNovaArea* TargetArea = FindArea(SourceLocation);

			// Detect enough time spent & valid target available
			if (TargetArea && CurrentTime - SpacecraftState.CurrentStateStartTime > FNovaTime::FromMinutes(StationWaitTime))
			{
				NLOG("UNovaAISimulationComponent::ProcessNavigation : '%s' undocking toward '%s'",
					*Identifier.ToString(EGuidFormats::Short), *TargetArea->Name.ToString());

				SetSpacecraftState(SpacecraftState, ENovaAISpacecraftState::Undocking);
			}

			// Detect maximum time spent
			else if (CurrentTime - SpacecraftState.CurrentStateStartTime > FNovaTime::FromMinutes(StationPatrolTimeout))
			{
				NLOG(
					"UNovaAISimulationComponent::ProcessNavigation : '%s' undocking for patrol", *Identifier.ToString(EGuidFormats::Short));

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

void UNovaAISimulationComponent::CreateGame()
{
	NLOG("UNovaAISimulationComponent::CreateGame");

	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		// Get game state pointers
		UNeutronAssetManager* AssetManager = UNeutronAssetManager::Get();
		NCHECK(AssetManager);
		ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
		NCHECK(GameState);

		// Get asset lists
		const class UNovaCelestialBody* DefaultPlanet = AssetManager->GetDefaultAsset<UNovaCelestialBody>();
		NCHECK(DefaultPlanet);
		TArray<const UNovaAISpacecraftDescription*> SpacecraftDescriptions = AssetManager->GetAssets<UNovaAISpacecraftDescription>();

		// Spawn spacecraft
		FRandomStream RandomStream;
		for (int32 Index = 0; Index < InitialTechnicalSpacecraftCount; Index++)
		{
			// Get the location
			const double InitialAltitude = RandomStream.FRandRange(InitialMinAltitude, InitialMaxAltitude);
			const double InitialPhase    = RandomStream.FRandRange(0.0, 360.0);
			FNovaOrbit   Orbit           = FNovaOrbit(FNovaOrbitGeometry(DefaultPlanet, InitialAltitude, InitialPhase), FNovaTime());

			// Get the class
			const UNovaAISpacecraftDescription* SpacecraftDescription =
				SpacecraftDescriptions[RandomStream.RandHelper(SpacecraftDescriptions.Num())];

			// Create the spacecraft
			FNovaSpacecraft Spacecraft = SpacecraftDescription->Spacecraft;
			Spacecraft.Name            = GetTechnicalShipName(RandomStream, Index);
			Spacecraft.SpacecraftClass = SpacecraftDescription;
			Spacecraft.Identifier      = FGuid::NewGuid();

			// Register the spacecraft
			SpacecraftDatabase.Add(Spacecraft.Identifier, FNovaAISpacecraftState());
			GameState->UpdateSpacecraft(Spacecraft, &Orbit);
		}
	}
}

FString UNovaAISimulationComponent::GetTechnicalShipName(FRandomStream& RandomStream, int32 Index) const
{
	FString Prefix = TechnicalNamePrefixes[RandomStream.RandHelper(TechnicalNamePrefixes.Num())];
	FString Suffix = TechnicalNameSuffixes[RandomStream.RandHelper(TechnicalNameSuffixes.Num())];
	return Prefix + " " + Suffix + " " + FString::FormatAsNumber(100 + Index);
}

void UNovaAISimulationComponent::SetSpacecraftState(FNovaAISpacecraftState& State, ENovaAISpacecraftState NewState)
{
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	FNovaTime CurrentTime = GameState->GetCurrentTime();

	State.CurrentState          = NewState;
	State.CurrentStateStartTime = CurrentTime;

	// GameState->SetTimeDilation(ENovaTimeDilation::Normal);
}

void UNovaAISimulationComponent::StartTrajectory(const FNovaOrbit& SourceOrbit, const FNovaOrbit& DestinationOrbit, FNovaTime DeltaTime,
	const TArray<FGuid>& Spacecraft, double ExplicitAltitude)
{
	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Compute the best staging altitude
	if (ExplicitAltitude == 0)
	{
		// Compute trajectory candidates
		TArray<FNovaTrajectory>   Candidates;
		FNovaTrajectoryParameters Parameters = OrbitalSimulation->PrepareTrajectory(SourceOrbit, DestinationOrbit, DeltaTime, Spacecraft);
		for (double Altitude = 300; Altitude <= 1500; Altitude += 200)
		{
			if (Altitude != Parameters.DestinationAltitude && Altitude != Parameters.Source.Geometry.StartAltitude &&
				Altitude != Parameters.Source.Geometry.OppositeAltitude)
			{
				FNovaTrajectory NewTrajectory = OrbitalSimulation->ComputeTrajectory(Parameters, Altitude);
				if (NewTrajectory.IsValid() && NewTrajectory.TotalTravelDuration.AsDays() < 20)
				{
					Candidates.Add(NewTrajectory);
				}
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

	// Use the provided altitude
	else
	{
		FNovaTrajectoryParameters Parameters = OrbitalSimulation->PrepareTrajectory(SourceOrbit, DestinationOrbit, DeltaTime, Spacecraft);
		FNovaTrajectory           NewTrajectory = OrbitalSimulation->ComputeTrajectory(Parameters, ExplicitAltitude);
		NCHECK(NewTrajectory.IsValid() && NewTrajectory.TotalTravelDuration.AsDays() < 20);
		OrbitalSimulation->CommitTrajectory(Spacecraft, NewTrajectory);
	}
}

const UNovaArea* UNovaAISimulationComponent::FindArea(const FNovaOrbitalLocation* SourceLocation) const
{
	NCHECK(SourceLocation != nullptr);

	// Get game state pointers
	UNeutronAssetManager* AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Pick a random destination that is not the nearest one
	TArray<const UNovaArea*> Areas           = AssetManager->GetAssets<UNovaArea>();
	auto                     AreaAndDistance = OrbitalSimulation->GetNearestAreaAndDistance(*SourceLocation);

	// Remove the nearest destination and all areas over quota
	Areas.Remove(AreaAndDistance.Key);
	for (const TPair<const UNovaArea*, int32>& AreaAndQuota : AreasQuotas)
	{
		if (AreaAndQuota.Value >= AreaAndQuota.Key->AIQuota)
		{
			Areas.Remove(AreaAndQuota.Key);
		}
	}

	return Areas.Num() > 0 ? Areas[FMath::RandHelper(Areas.Num())] : nullptr;
}

void UNovaAISimulationComponent::FindPatrolOrbit(FNovaOrbit& DestinationOrbit) const
{
	// Get game state pointers
	UNeutronAssetManager* AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);
	const UNovaCelestialBody* Body = AssetManager->GetDefaultAsset<UNovaCelestialBody>();

	// Generate a random orbit
	const double Altitude = PatrolRandomStream.FRandRange(PatrolMinAltitude, PatrolMaxAltitude);
	const double Phase    = PatrolRandomStream.FRandRange(0.0, 360.0);
	DestinationOrbit      = FNovaOrbit(FNovaOrbitGeometry(Body, Altitude, Phase), FNovaTime());
}

#undef LOCTEXT_NAMESPACE
