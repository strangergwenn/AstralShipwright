// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Nova/Spacecraft/NovaSpacecraft.h"
#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "UNovaOrbitalSimulationComponent"

/*----------------------------------------------------
    Internal structures
----------------------------------------------------*/

/** Hohmann transfer orbit parameters */
struct FNovaHohmannTransfer
{
	/** Compute a Hohmann transfer from the elliptic orbit (ManeuverRadius, OriginalRadius) to the circular orbit DestinationRadius,
	 * raising OriginalRadius to DestinationRadius, while maneuvering at ManeuverRadius (technically not a Hohmann transfer) */
	FNovaHohmannTransfer(const double µ, const double ManeuverRadius, const double OriginalRadius, const double DestinationRadius)
	{
		const double SourceSemiMajorAxis = 0.5f * (ManeuverRadius + OriginalRadius);

#if 1
		// StartDeltaV = VTransfer1 - VSource
		StartDeltaV = sqrt((2.0 * µ * DestinationRadius) / (ManeuverRadius * (ManeuverRadius + DestinationRadius))) -
					  sqrt(µ * ((2.0 / ManeuverRadius) - (1.0 / SourceSemiMajorAxis)));
#else
		// StartDeltaV = VTransfer1 - VSource, factorized with VSource
		StartDeltaV = sqrt(µ / ManeuverRadius) * (sqrt((2.0 * DestinationRadius) / (ManeuverRadius + DestinationRadius)) - 1.0);
#endif

		// EndDeltaV = VDest - VTransfer2
		EndDeltaV = sqrt(µ / DestinationRadius) * (1.0 - sqrt((2.0 * ManeuverRadius) / (ManeuverRadius + DestinationRadius)));

		TotalDeltaV = abs(StartDeltaV) + abs(EndDeltaV);

		Duration = PI * sqrt(pow(ManeuverRadius + DestinationRadius, 3.0) / (8.0 * µ)) / 60;
	}

	double StartDeltaV;
	double EndDeltaV;
	double TotalDeltaV;
	double Duration;
};

/** Trajectory computation parameters */
struct FNovaTrajectoryParameters
{
	double        StartTime;
	FNovaOrbit    Source;
	double        DestinationAltitude;
	double        DestinationPhase;
	TArray<FGuid> SpacecraftIdentifiers;

	const UNovaPlanet* Planet;
	double             µ;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaOrbitalSimulationComponent::UNovaOrbitalSimulationComponent() : Super()
{
	// Settings
	SetIsReplicatedByDefault(true);

	// Defaults
	OrbitGarbageCollectionDelay = 1.0f;
	TimeMarginBeforeManeuver    = 1.0f;
}

/*----------------------------------------------------
    General simulation
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::BeginPlay()
{
	Super::BeginPlay();

	Areas = GetOwner()->GetGameInstance<UNovaGameInstance>()->GetAssetManager()->GetAssets<UNovaArea>();
}

void UNovaOrbitalSimulationComponent::UpdateSimulation()
{
	// Clean up our local state
	TimeOfNextPlayerManeuver = DBL_MAX;
	SpacecraftOrbitDatabase.UpdateCache();
	SpacecraftTrajectoryDatabase.UpdateCache();

	// Run processes
	ProcessOrbitCleanup();
	ProcessAreas();
	ProcessSpacecraftOrbits();
	ProcessSpacecraftTrajectories();
}

double UNovaOrbitalSimulationComponent::GetCurrentTime() const
{
	return Cast<ANovaGameState>(GetOwner())->GetCurrentTime();
}

/*----------------------------------------------------
    Trajectory & orbiting interface
----------------------------------------------------*/

TSharedPtr<FNovaTrajectoryParameters> UNovaOrbitalSimulationComponent::PrepareTrajectory(const TSharedPtr<FNovaOrbit>& Source,
	const TSharedPtr<FNovaOrbit>& Destination, double DeltaTime, const TArray<FGuid>& SpacecraftIdentifiers) const
{
	TSharedPtr<FNovaTrajectoryParameters> Parameters = MakeShared<FNovaTrajectoryParameters>();

	NCHECK(Source.IsValid() && Destination.IsValid());
	NCHECK(Source->IsValid() && Destination->IsValid());
	NCHECK(*Source.Get() != *Destination.Get());
	NCHECK(Destination->Geometry.IsCircular());
	NCHECK(Source->Geometry.Planet == Destination->Geometry.Planet);
	NCHECK(SpacecraftIdentifiers.Num() > 0);

	// Get basic parameters
	Parameters->StartTime             = GetCurrentTime() + DeltaTime;
	Parameters->Source                = *Source;
	Parameters->DestinationPhase      = Destination->GetCurrentPhase<true>(GetCurrentTime() + DeltaTime);
	Parameters->DestinationAltitude   = Destination->Geometry.StartAltitude;
	Parameters->SpacecraftIdentifiers = SpacecraftIdentifiers;

	// Get orbital parameters
	Parameters->Planet = Source->Geometry.Planet;
	Parameters->µ      = Source->Geometry.Planet->GetGravitationalParameter();

	return Parameters;
}

TSharedPtr<FNovaTrajectory> UNovaOrbitalSimulationComponent::ComputeTrajectory(
	const TSharedPtr<FNovaTrajectoryParameters>& Parameters, float PhasingAltitude)
{
	// Get phase and altitude
	const double& StartTime           = Parameters->StartTime;
	double        SourceAltitudeA     = Parameters->Source.Geometry.StartAltitude;
	double        SourceAltitudeB     = Parameters->Source.Geometry.StartAltitude;
	double        SourcePhase         = Parameters->Source.GetCurrentPhase<true>(StartTime);
	const double& DestinationAltitude = Parameters->DestinationAltitude;
	const double& DestinationPhase    = Parameters->DestinationPhase;

	// if the source orbit isn't circular, circularize to PhasingAltitude at one of the apsides
	double InitialWaitingDuration = 0;
	if (!Parameters->Source.Geometry.IsCircular())
	{
		const FNovaOrbitGeometry& Geometry = Parameters->Source.Geometry;

		const bool CircularizeAtStart =
			FMath::Abs(Geometry.OppositeAltitude - PhasingAltitude) < FMath::Abs(Geometry.StartAltitude - PhasingAltitude);

		// Get the basic circularization parameters
		const double InitialSourcePhase = SourcePhase;
		SourcePhase                     = CircularizeAtStart ? Geometry.StartPhase : Geometry.StartPhase + 180;
		SourceAltitudeA                 = CircularizeAtStart ? Geometry.StartAltitude : Geometry.OppositeAltitude;
		SourceAltitudeB                 = CircularizeAtStart ? Geometry.OppositeAltitude : Geometry.StartAltitude;
		const double WaitingPhaseDelta  = fmod(SourcePhase - InitialSourcePhase + 360.0, 360.0);
		InitialWaitingDuration          = (WaitingPhaseDelta / 360.0) * Geometry.GetOrbitalPeriod();
	}

	// Get orbital parameters
	const double R1A = Parameters->Planet->GetRadius(SourceAltitudeA);
	const double R1B = Parameters->Planet->GetRadius(SourceAltitudeB);
	const double R2  = Parameters->Planet->GetRadius(PhasingAltitude);
	const double R3  = Parameters->Planet->GetRadius(DestinationAltitude);

	// Compute both Hohmann transfers as well as the orbital periods
	const FNovaHohmannTransfer TransferA(Parameters->µ, R1A, R1B, R2);
	const FNovaHohmannTransfer TransferB(Parameters->µ, R2, R2, R3);
	const double&              PhasingOrbitPeriod     = GetOrbitalPeriod(Parameters->µ, R2);
	const double&              DestinationOrbitPeriod = GetOrbitalPeriod(Parameters->µ, R3);

	// Compute the new destination parameters after both transfers, ignoring the phasing orbit
	const double TotalTransferDuration                = InitialWaitingDuration + TransferA.Duration + TransferB.Duration;
	const double DestinationPhaseChangeDuringTransfer = (TotalTransferDuration / DestinationOrbitPeriod) * 360.0;
	const double NewDestinationPhaseAfterTransfers    = fmod(DestinationPhase + DestinationPhaseChangeDuringTransfer, 360.0);
	double       PhaseDelta                           = fmod(NewDestinationPhaseAfterTransfers - SourcePhase + 360.0, 360.0);
	if (PhasingOrbitPeriod > DestinationOrbitPeriod)
	{
		PhaseDelta = PhaseDelta - 360.0;
	}

	// Compute the time spent waiting
	const double PhasingDuration     = PhaseDelta / (360.0 * (1.0 / PhasingOrbitPeriod - 1.0 / DestinationOrbitPeriod));
	const double PhasingAngle        = (PhasingDuration / PhasingOrbitPeriod) * 360.0;
	const double TotalTravelDuration = TotalTransferDuration + PhasingDuration;

	// Get a list of spacecraft
	TArray<const FNovaSpacecraft*> Spacecraft;
	for (const FGuid& Identifier : Parameters->SpacecraftIdentifiers)
	{
		const FNovaSpacecraft* NewSpacecraft = Cast<ANovaGameState>(GetOwner())->GetSpacecraft(Identifier);
		NCHECK(NewSpacecraft != nullptr);
		Spacecraft.Add(NewSpacecraft);
	}

	// Sort spacecraft by least acceleration because the fleet will match the least-performing
	Spacecraft.Sort(
		[](const FNovaSpacecraft& A, const FNovaSpacecraft& B)
		{
			return A.GetPropulsionMetrics().GetLowestAcceleration() < B.GetPropulsionMetrics().GetLowestAcceleration();
		});

	// Start building trajectory
	TSharedPtr<FNovaTrajectory>             Trajectory        = MakeShared<FNovaTrajectory>();
	double                                  CurrentTime       = StartTime + InitialWaitingDuration;
	double                                  CurrentPhase      = SourcePhase;
	float                                   CurrentPropellant = Spacecraft[0]->GetRemainingPropellantMass();
	const FNovaSpacecraftPropulsionMetrics& Metrics           = Spacecraft[0]->GetPropulsionMetrics();

	// Departure burn on first transfer
	double ManeuverDuration     = Metrics.GetManeuverDurationAndPropellantUsed(TransferA.StartDeltaV, CurrentPropellant);
	bool   FirstTransferIsValid = Trajectory->Add(FNovaManeuver(TransferA.StartDeltaV, CurrentPhase, CurrentTime, ManeuverDuration));

	// First transfer
	if (FirstTransferIsValid)
	{
		Trajectory->Add(FNovaOrbit(
			FNovaOrbitGeometry(Parameters->Planet, SourceAltitudeA, PhasingAltitude, CurrentPhase, CurrentPhase + 180), CurrentTime));
	}
	CurrentPhase += 180;

	// Circularization burn after first transfer
	ManeuverDuration          = Metrics.GetManeuverDurationAndPropellantUsed(TransferA.EndDeltaV, CurrentPropellant);
	double ManeuverPhaseDelta = ((ManeuverDuration / 2.0) / PhasingOrbitPeriod) * 360.0;
	Trajectory->Add(FNovaManeuver(TransferA.EndDeltaV, CurrentPhase - ManeuverPhaseDelta,
		CurrentTime + TransferA.Duration - ManeuverDuration / 2.0, ManeuverDuration));
	CurrentTime += TransferA.Duration;

	// Phasing orbit
	if (FirstTransferIsValid)
	{
		Trajectory->Add(
			FNovaOrbit(FNovaOrbitGeometry(Parameters->Planet, PhasingAltitude, PhasingAltitude, CurrentPhase, CurrentPhase + PhasingAngle),
				CurrentTime));
	}
	CurrentTime += PhasingDuration;
	CurrentPhase += PhasingAngle;

	// Departure burn on second transfer, accounting for whether the departure burn occurs in the middle of the arc or just after
	ManeuverDuration = Metrics.GetManeuverDurationAndPropellantUsed(TransferB.StartDeltaV, CurrentPropellant);
	if (FirstTransferIsValid)
	{
		ManeuverPhaseDelta = ((ManeuverDuration / 2.0) / PhasingOrbitPeriod) * 360.0;
		Trajectory->Add(FNovaManeuver(
			TransferB.StartDeltaV, CurrentPhase - ManeuverPhaseDelta, CurrentTime - ManeuverDuration / 2.0, ManeuverDuration));
	}
	else
	{
		Trajectory->Add(FNovaManeuver(TransferB.StartDeltaV, CurrentPhase, CurrentTime, ManeuverDuration));
	}

	// Second transfer
	Trajectory->Add(FNovaOrbit(
		FNovaOrbitGeometry(Parameters->Planet, PhasingAltitude, DestinationAltitude, CurrentPhase, CurrentPhase + 180), CurrentTime));
	ManeuverDuration = Metrics.GetManeuverDurationAndPropellantUsed(TransferB.EndDeltaV, CurrentPropellant);
	CurrentPhase += 180;

	// Circularization burn after second transfer
	ManeuverPhaseDelta = (ManeuverDuration / PhasingOrbitPeriod) * 360.0;
	Trajectory->Add(FNovaManeuver(
		TransferB.EndDeltaV, CurrentPhase - ManeuverPhaseDelta, CurrentTime + TransferB.Duration - ManeuverDuration, ManeuverDuration));

	// Metadata
	Trajectory->TotalTravelDuration = TotalTravelDuration;
	Trajectory->TotalDeltaV         = TransferA.TotalDeltaV + TransferB.TotalDeltaV;

#if WITH_EDITOR

	// Confirm the final spacecraft phase matches the destination's
	const double DestinationPhasingAngle = (PhasingDuration / PhasingOrbitPeriod) * 360.0;
	const double FinalDestinationPhase   = fmod(DestinationPhase + (TotalTravelDuration / DestinationOrbitPeriod) * 360, 360.0);
	const double FinalSpacecraftPhase    = fmod(SourcePhase + PhasingAngle, 360.0);

#if 0
	NLOG("--------------------------------------------------------------------------------");
	NLOG("UNovaOrbitalSimulationComponent::ComputeTrajectory : (%f, %f) --> (%f, %f)", SourceAltitudeA, SourcePhase, DestinationAltitude,
		DestinationPhase);
	NLOG("Transfer A : DVS %f, DVE %f, DV %f, T %f", TransferA.StartDeltaV, TransferA.EndDeltaV, TransferA.TotalDeltaV);
	NLOG("Transfer B : DVS %f, DVE %f, DV %f, T %f", TransferB.StartDeltaV, TransferB.EndDeltaV, TransferB.TotalDeltaV);
	NLOG("DestinationPhaseChangeDuringTransfer %f, NewDestinationPhaseAfterTransfers %f, PhaseDelta %f",
		DestinationPhaseChangeDuringTransfer, NewDestinationPhaseAfterTransfers, PhaseDelta);
	NLOG("PhasingOrbitPeriod = %f, DestinationOrbitPeriod = %f", PhasingOrbitPeriod, DestinationOrbitPeriod);
	NLOG("PhasingDuration = %f, PhasingAngle = %f", PhasingDuration, PhasingAngle);
	NLOG("FinalDestinationPhase = %f, FinalSpacecraftPhase = %f", FinalDestinationPhase, FinalSpacecraftPhase);
	NLOG("Trajectory->GetStartTime() = %f, Parameters->StartTime = %f", FMath::IsFinite(PhasingDuration) ? Trajectory->GetStartTime() : 0,
		Parameters->StartTime);
	NLOG("--------------------------------------------------------------------------------");
#endif

	NCHECK(FMath::Abs(FinalSpacecraftPhase - FinalDestinationPhase) < SMALL_NUMBER);
	if (FMath::IsFinite(PhasingDuration))
	{
		NCHECK(FMath::Abs(Trajectory->GetStartTime() - Parameters->StartTime) * 60.0 < 1);
	}

#endif

	return Trajectory;
}

bool UNovaOrbitalSimulationComponent::IsOnStartedTrajectory(const FGuid& SpacecraftIdentifier) const
{
	const FNovaTrajectory* Trajectory = SpacecraftTrajectoryDatabase.Get(SpacecraftIdentifier);

	return Trajectory && GetCurrentTime() >= Trajectory->GetFirstManeuverStartTime() && GetCurrentTime() <= Trajectory->GetArrivalTime();
}

bool UNovaOrbitalSimulationComponent::CanStartTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory) const
{
	return GetOwner()->GetLocalRole() == ROLE_Authority && Trajectory.IsValid() && Trajectory->IsValid() &&
		   Trajectory->Maneuvers[0].Time > GetCurrentTime();
}

void UNovaOrbitalSimulationComponent::StartTrajectory(
	const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
	NCHECK(Trajectory.IsValid() && Trajectory->IsValid());

	NLOG("UNovaOrbitalSimulationComponent::StartTrajectory for %d spacecraft", SpacecraftIdentifiers.Num());

	// Move spacecraft to the trajectory
	SpacecraftTrajectoryDatabase.Add(SpacecraftIdentifiers, Trajectory);
}

void UNovaOrbitalSimulationComponent::CompleteTrajectory(const TArray<FGuid>& SpacecraftIdentifiers)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
	NLOG("UNovaOrbitalSimulationComponent::CompleteTrajectory for %d spacecraft", SpacecraftIdentifiers.Num());

	FNovaOrbit CommonFinalOrbit;
	bool       FoundCommonOrbit = false;

	// Compute the final orbit and ensure all spacecraft are going there
	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		const FNovaTrajectory* Trajectory = SpacecraftTrajectoryDatabase.Get(Identifier);
		if (Trajectory)
		{
			const FNovaOrbit FinalOrbit = Trajectory->GetFinalOrbit();
			NCHECK(!FoundCommonOrbit || FinalOrbit == CommonFinalOrbit);
			FoundCommonOrbit = true;
			CommonFinalOrbit = FinalOrbit;
		}
	}

	// Commit the change
	NCHECK(FoundCommonOrbit);
	SetOrbit(SpacecraftIdentifiers, MakeShared<FNovaOrbit>(CommonFinalOrbit));
}

void UNovaOrbitalSimulationComponent::SetOrbit(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
	NCHECK(Orbit.IsValid());
	NCHECK(Orbit->IsValid());

	NLOG("UNovaOrbitalSimulationComponent::SetOrbit for %d spacecraft", SpacecraftIdentifiers.Num());

	SpacecraftTrajectoryDatabase.Remove(SpacecraftIdentifiers);
	SpacecraftOrbitDatabase.Add(SpacecraftIdentifiers, Orbit);
}

void UNovaOrbitalSimulationComponent::MergeOrbit(const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaOrbit>& Orbit)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
	NCHECK(Orbit.IsValid());
	NCHECK(Orbit->IsValid());

	NLOG("UNovaOrbitalSimulationComponent::MergeOrbit for %d spacecraft", SpacecraftIdentifiers.Num());

	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		SpacecraftOrbitDatabase.Remove({Identifier});
	}

	SetOrbit(SpacecraftIdentifiers, Orbit);
}

/*----------------------------------------------------
    Trajectory & orbiting getters
----------------------------------------------------*/

UNovaOrbitalSimulationComponent* UNovaOrbitalSimulationComponent::Get(const UObject* Outer)
{
	const ANovaGameState* GameState = Outer->GetWorld()->GetGameState<ANovaGameState>();
	if (IsValid(GameState))
	{
		UNovaOrbitalSimulationComponent* SimulationComponent = GameState->GetOrbitalSimulation();
		if (IsValid(SimulationComponent))
		{
			return SimulationComponent;
		}
	}

	return nullptr;
}

const FNovaOrbit* UNovaOrbitalSimulationComponent::GetPlayerOrbit() const
{
	const ANovaGameState* GameState        = GetOwner<ANovaGameState>();
	const FGuid&          PlayerIdentifier = GameState->GetPlayerSpacecraftIdentifier();

	if (PlayerIdentifier.IsValid())
	{
		return GetSpacecraftOrbit(PlayerIdentifier);
	}
	else
	{
		return nullptr;
	}
}

const FNovaTrajectory* UNovaOrbitalSimulationComponent::GetPlayerTrajectory() const
{
	const ANovaGameState* GameState        = GetOwner<ANovaGameState>();
	const FGuid&          PlayerIdentifier = GameState->GetPlayerSpacecraftIdentifier();

	if (PlayerIdentifier.IsValid())
	{
		return GetSpacecraftTrajectory(PlayerIdentifier);
	}
	else
	{
		return nullptr;
	}
}

const FNovaOrbitalLocation* UNovaOrbitalSimulationComponent::GetPlayerLocation() const
{
	const ANovaGameState* GameState        = GetOwner<ANovaGameState>();
	const FGuid&          PlayerIdentifier = GameState->GetPlayerSpacecraftIdentifier();

	if (PlayerIdentifier.IsValid())
	{
		return GetSpacecraftLocation(PlayerIdentifier);
	}
	else
	{
		return nullptr;
	}
}

TPair<const UNovaArea*, float> UNovaOrbitalSimulationComponent::GetNearestAreaAndDistance(const FNovaOrbitalLocation& Location) const
{
	const UNovaArea* ClosestArea     = nullptr;
	float            ClosestDistance = MAX_FLT;

	for (const TPair<const UNovaArea*, FNovaOrbitalLocation>& Entry : AreaOrbitalLocations)
	{
		float Distance = Entry.Value.GetDistanceTo(Location);
		if (Distance < ClosestDistance)
		{
			ClosestArea     = Entry.Key;
			ClosestDistance = Distance;
		}
	}

	return TPair<const UNovaArea*, float>(ClosestArea, ClosestDistance);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::ProcessOrbitCleanup()
{
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		// We need orbit data right until the time a trajectory actually start, so we remove it there
		for (const FNovaTrajectoryDatabaseEntry& DatabaseEntry : SpacecraftTrajectoryDatabase.Get())
		{
			if (GetCurrentTime() >= DatabaseEntry.Trajectory.GetFirstManeuverStartTime() + OrbitGarbageCollectionDelay)
			{
				SpacecraftOrbitDatabase.Remove(DatabaseEntry.Identifiers);
			}
		}
	}
}

void UNovaOrbitalSimulationComponent::ProcessAreas()
{
	for (const UNovaArea* Area : Areas)
	{
		// Update the position
		double CurrentPhase = GetAreaOrbit(Area)->Geometry.GetCurrentPhase<true>(GetCurrentTime());

#if 0
		NLOG("UNovaOrbitalSimulationComponent::ProcessSpacecraftOrbits : %s has phase %f", *Area->Name.ToString(), CurrentPhase);
#endif

		// Add or update the current orbit and position
		FNovaOrbitalLocation* Entry = AreaOrbitalLocations.Find(Area);
		if (Entry)
		{
			Entry->Phase = CurrentPhase;
		}
		else
		{
			AreaOrbitalLocations.Add(Area, FNovaOrbitalLocation(GetAreaOrbit(Area)->Geometry, CurrentPhase));
		}
	}
}

void UNovaOrbitalSimulationComponent::ProcessSpacecraftOrbits()
{
	for (const FNovaOrbitDatabaseEntry& DatabaseEntry : SpacecraftOrbitDatabase.Get())
	{
		// Update the position
		double                     CurrentPhase = DatabaseEntry.Orbit.GetCurrentPhase<true>(GetCurrentTime());
		const FNovaOrbitalLocation NewLocation  = FNovaOrbitalLocation(DatabaseEntry.Orbit.Geometry, CurrentPhase);

#if 0
		NLOG("UNovaOrbitalSimulationComponent::ProcessSpacecraftOrbits : %s has phase %f, sphase %f, ephase %f",
			*DatabaseEntry.Identifiers[0].ToString(), NewLocation.Phase, NewLocation.Geometry.StartPhase, NewLocation.Geometry.EndPhase);
#endif

		// Add or update the current orbit and position
		for (const FGuid& Identifier : DatabaseEntry.Identifiers)
		{
			FNovaOrbitalLocation* Entry = SpacecraftOrbitalLocations.Find(Identifier);
			if (Entry)
			{
				*Entry = NewLocation;
			}
			else
			{
				SpacecraftOrbitalLocations.Add(Identifier, NewLocation);
			}
		}
	}
}

void UNovaOrbitalSimulationComponent::ProcessSpacecraftTrajectories()
{
	TArray<TArray<FGuid>> CompletedTrajectories;

	for (const FNovaTrajectoryDatabaseEntry& DatabaseEntry : SpacecraftTrajectoryDatabase.Get())
	{
		if (GetCurrentTime() >= DatabaseEntry.Trajectory.GetFirstManeuverStartTime())
		{
			// Compute the new location
			FNovaOrbitalLocation NewLocation = DatabaseEntry.Trajectory.GetCurrentLocation(GetCurrentTime());
			if (!NewLocation.IsValid())
			{
				NLOG("UNovaOrbitalSimulationComponent::ProcessSpacecraftTrajectories : missing trajectory data");
			}

#if 0
			NLOG("UNovaOrbitalSimulationComponent::ProcessSpacecraftTrajectories : %s has phase %f, with sphase %f, ephase %f",
				*DatabaseEntry.Identifiers[0].ToString(), NewLocation.Phase, NewLocation.Geometry.StartPhase,
				NewLocation.Geometry.EndPhase);
#endif

			// Add or update the current orbit and location
			for (const FGuid& Identifier : DatabaseEntry.Identifiers)
			{
				FNovaOrbitalLocation* Entry = SpacecraftOrbitalLocations.Find(Identifier);
				if (Entry)
				{
					*Entry = NewLocation;
				}
				else
				{
					SpacecraftOrbitalLocations.Add(Identifier, NewLocation);
				}
			}

			// Complete the trajectory on arrival
			if (GetCurrentTime() > DatabaseEntry.Trajectory.GetArrivalTime())
			{
				CompletedTrajectories.Add(DatabaseEntry.Identifiers);
			}
		}

		// If this trajectory is for a player spacecraft, detect whether we're nearing a maneuver
		const ANovaGameState* GameState        = GetOwner<ANovaGameState>();
		const FGuid&          PlayerIdentifier = GameState->GetPlayerSpacecraftIdentifier();
		if (PlayerIdentifier.IsValid() && DatabaseEntry.Identifiers.Contains(PlayerIdentifier))
		{
			TimeOfNextPlayerManeuver = DatabaseEntry.Trajectory.GetNextManeuverStartTime(GetCurrentTime());
		}
	}

	// Complete trajectories
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		for (const TArray<FGuid>& Identifiers : CompletedTrajectories)
		{
			NLOG("UNovaOrbitalSimulationComponent::ProcessSpacecraftTrajectories : completing trajectory for %d spacecraft at time %f",
				Identifiers.Num(), GetCurrentTime());
			CompleteTrajectory(Identifiers);
		}
	}
}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaOrbitalSimulationComponent, SpacecraftOrbitDatabase);
	DOREPLIFETIME(UNovaOrbitalSimulationComponent, SpacecraftTrajectoryDatabase);
}

#undef LOCTEXT_NAMESPACE
