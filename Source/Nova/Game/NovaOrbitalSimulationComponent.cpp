// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Nova/Spacecraft/NovaSpacecraft.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"
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

		Duration = FNovaTime::FromMinutes(PI * sqrt(pow(ManeuverRadius + DestinationRadius, 3.0) / (8.0 * µ)) / 60);
	}

	double    StartDeltaV;
	double    EndDeltaV;
	double    TotalDeltaV;
	FNovaTime Duration;
};

/** Trajectory computation parameters */
struct FNovaTrajectoryParameters
{
	FNovaTime     StartTime;
	FNovaOrbit    Source;
	double        DestinationAltitude;
	double        DestinationPhase;
	TArray<FGuid> SpacecraftIdentifiers;

	const UNovaCelestialBody* Body;
	double                    µ;
};

/** Results of a maneuver on a fleet of spacecraft */
struct FNovaSpacecraftFleetManeuver
{
	FNovaSpacecraftFleetManeuver(FNovaTime D, const TArray<float>& TF) : Duration(D), ThrustFactors(TF)
	{}

	FNovaTime     Duration;
	TArray<float> ThrustFactors;
};

/** Structure representing a fleet of spacecraft */
struct FNovaSpacecraftFleet
{
	struct FNovaSpacecraftFleetEntry
	{
		FNovaSpacecraftFleetEntry(const FNovaSpacecraft* Spacecraft, const ANovaGameState* GameState)
		{
			Metrics = Spacecraft->GetPropulsionMetrics();

			// The core assumption here is that only maneuvers can modify mass, and so the current mass won't change until the next
			// maneuver. The practical consequence is that trajectories can only ever be plotted while undocked
			// and any non-propulsion-related transfer of mass should abort the trajectory
			CurrentCargoMass = Spacecraft->GetCurrentCargoMass();
			CurrentPropellantMass =
				Spacecraft->FindComponentByClass<UNovaSpacecraftPropellantSystem>(GameState)->GetCurrentPropellantMass();
		}

		FNovaSpacecraftPropulsionMetrics Metrics;
		float                            CurrentCargoMass;
		float                            CurrentPropellantMass;
	};

	FNovaSpacecraftFleet(const TArray<FGuid>& SpacecraftIdentifiers, const ANovaGameState* GameState)
	{
		for (const FGuid& Identifier : SpacecraftIdentifiers)
		{
			const FNovaSpacecraft* NewSpacecraft = GameState->GetSpacecraft(Identifier);
			NCHECK(NewSpacecraft != nullptr);
			Fleet.Add(FNovaSpacecraftFleetEntry(NewSpacecraft, GameState));
		}
	}

	FNovaSpacecraftFleetManeuver AddManeuver(double DeltaV)
	{
		// Update the propellant use and compute individual maneuver durations for each ship
		FNovaTime         MaxDuration;
		TArray<FNovaTime> Durations;
		for (FNovaSpacecraftFleetEntry& Entry : Fleet)
		{
			FNovaTime Duration =
				Entry.Metrics.GetManeuverDurationAndPropellantUsed(DeltaV, Entry.CurrentCargoMass, Entry.CurrentPropellantMass);
			Durations.Add(Duration);

			if (Duration > MaxDuration)
			{
				MaxDuration = Duration;
			}
		}

		// Adapt each ship's thrust to the longest duration
		TArray<float> ThrustFactors;
		for (FNovaTime Duration : Durations)
		{
			ThrustFactors.Add(Duration / MaxDuration);
		}

		return FNovaSpacecraftFleetManeuver(MaxDuration, ThrustFactors);
	}

	TArray<FNovaSpacecraftFleetEntry> Fleet;
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
	TimeOfNextPlayerManeuver = FNovaTime::FromMinutes(DBL_MAX);
	SpacecraftOrbitDatabase.UpdateCache();
	SpacecraftTrajectoryDatabase.UpdateCache();

	// Run processes
	ProcessOrbitCleanup();
	ProcessAreas();
	ProcessSpacecraftOrbits();
	ProcessSpacecraftTrajectories();
}

FNovaTime UNovaOrbitalSimulationComponent::GetCurrentTime() const
{
	return GetOwner<ANovaGameState>()->GetCurrentTime();
}

/*----------------------------------------------------
    Trajectory & orbiting interface
----------------------------------------------------*/

TSharedPtr<FNovaTrajectoryParameters> UNovaOrbitalSimulationComponent::PrepareTrajectory(const TSharedPtr<FNovaOrbit>& Source,
	const TSharedPtr<FNovaOrbit>& Destination, FNovaTime DeltaTime, const TArray<FGuid>& SpacecraftIdentifiers) const
{
	TSharedPtr<FNovaTrajectoryParameters> Parameters = MakeShared<FNovaTrajectoryParameters>();

	NCHECK(Source.IsValid() && Destination.IsValid());
	NCHECK(Source->IsValid() && Destination->IsValid());
	NCHECK(*Source.Get() != *Destination.Get());
	NCHECK(Destination->Geometry.IsCircular());
	NCHECK(Source->Geometry.Body == Destination->Geometry.Body);
	NCHECK(SpacecraftIdentifiers.Num() > 0);

	// Get basic parameters
	Parameters->StartTime             = GetCurrentTime() + DeltaTime;
	Parameters->Source                = *Source;
	Parameters->DestinationPhase      = Destination->GetCurrentPhase<true>(GetCurrentTime() + DeltaTime);
	Parameters->DestinationAltitude   = Destination->Geometry.StartAltitude;
	Parameters->SpacecraftIdentifiers = SpacecraftIdentifiers;

	// Get orbital parameters
	Parameters->Body = Source->Geometry.Body;
	Parameters->µ    = Source->Geometry.Body->GetGravitationalParameter();

	return Parameters;
}

TSharedPtr<FNovaTrajectory> UNovaOrbitalSimulationComponent::ComputeTrajectory(
	const TSharedPtr<FNovaTrajectoryParameters>& Parameters, float PhasingAltitude)
{
	// Get phase and altitude
	const FNovaTime& StartTime           = Parameters->StartTime;
	double           SourceAltitudeA     = Parameters->Source.Geometry.StartAltitude;
	double           SourceAltitudeB     = Parameters->Source.Geometry.StartAltitude;
	double           SourcePhase         = Parameters->Source.GetCurrentPhase<true>(StartTime);
	const double     DestinationAltitude = Parameters->DestinationAltitude;
	const double     DestinationPhase    = Parameters->DestinationPhase;

	// if the source orbit isn't circular, circularize to PhasingAltitude at one of the apsides
	FNovaTime InitialWaitingDuration;
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
	const double R1A = Parameters->Body->GetRadius(SourceAltitudeA);
	const double R1B = Parameters->Body->GetRadius(SourceAltitudeB);
	const double R2  = Parameters->Body->GetRadius(PhasingAltitude);
	const double R3  = Parameters->Body->GetRadius(DestinationAltitude);

	// Compute both Hohmann transfers as well as the orbital periods
	const FNovaHohmannTransfer TransferA(Parameters->µ, R1A, R1B, R2);
	const FNovaHohmannTransfer TransferB(Parameters->µ, R2, R2, R3);
	const FNovaTime            PhasingOrbitPeriod     = GetOrbitalPeriod(Parameters->µ, R2);
	const FNovaTime            DestinationOrbitPeriod = GetOrbitalPeriod(Parameters->µ, R3);

	// Compute the new destination parameters after both transfers, ignoring the phasing orbit
	const FNovaTime TotalTransferDuration                = InitialWaitingDuration + TransferA.Duration + TransferB.Duration;
	const double    DestinationPhaseChangeDuringTransfer = (TotalTransferDuration / DestinationOrbitPeriod) * 360.0;
	const double    NewDestinationPhaseAfterTransfers    = fmod(DestinationPhase + DestinationPhaseChangeDuringTransfer, 360.0);
	double          PhaseDelta                           = fmod(NewDestinationPhaseAfterTransfers - SourcePhase + 360.0, 360.0);
	if (PhasingOrbitPeriod > DestinationOrbitPeriod)
	{
		PhaseDelta = PhaseDelta - 360.0;
	}

	// Compute the time spent waiting
	const FNovaTime PhasingDuration     = PhaseDelta / (360.0 * (1.0 / PhasingOrbitPeriod - 1.0 / DestinationOrbitPeriod));
	const double    PhasingAngle        = (PhasingDuration / PhasingOrbitPeriod) * 360.0;
	const FNovaTime TotalTravelDuration = TotalTransferDuration + PhasingDuration;

	// Start building trajectory
	FNovaSpacecraftFleet        Fleet(Parameters->SpacecraftIdentifiers, Cast<ANovaGameState>(GetOwner()));
	TSharedPtr<FNovaTrajectory> Trajectory   = MakeShared<FNovaTrajectory>();
	FNovaTime                   CurrentTime  = StartTime + InitialWaitingDuration;
	double                      CurrentPhase = SourcePhase;

	// Departure burn on first transfer
	FNovaSpacecraftFleetManeuver FleetManeuver        = Fleet.AddManeuver(TransferA.StartDeltaV);
	bool                         FirstTransferIsValid = Trajectory->Add(
        FNovaManeuver(TransferA.StartDeltaV, CurrentPhase, CurrentTime, FleetManeuver.Duration, FleetManeuver.ThrustFactors));

	// First transfer
	if (FirstTransferIsValid)
	{
		Trajectory->Add(FNovaOrbit(
			FNovaOrbitGeometry(Parameters->Body, SourceAltitudeA, PhasingAltitude, CurrentPhase, CurrentPhase + 180), CurrentTime));
	}
	CurrentPhase += 180;

	// Circularization burn after first transfer
	FleetManeuver                    = Fleet.AddManeuver(TransferA.EndDeltaV);
	double        ManeuverPhaseDelta = ((FleetManeuver.Duration / 2.0) / PhasingOrbitPeriod) * 360.0;
	FNovaManeuver Maneuver           = FNovaManeuver(TransferA.EndDeltaV, CurrentPhase - ManeuverPhaseDelta,
        CurrentTime + TransferA.Duration - FleetManeuver.Duration / 2.0, FleetManeuver.Duration, FleetManeuver.ThrustFactors);
	Trajectory->Add(Maneuver);
	CurrentTime += TransferA.Duration;

	// Phasing orbit
	if (FirstTransferIsValid)
	{
		Trajectory->Add(
			FNovaOrbit(FNovaOrbitGeometry(Parameters->Body, PhasingAltitude, PhasingAltitude, CurrentPhase, CurrentPhase + PhasingAngle),
				CurrentTime));
	}
	CurrentTime += PhasingDuration;
	CurrentPhase += PhasingAngle;

	// Departure burn on second transfer, accounting for whether the departure burn occurs in the middle of the arc or just after
	FleetManeuver = Fleet.AddManeuver(TransferB.StartDeltaV);
	if (FirstTransferIsValid)
	{
		ManeuverPhaseDelta = ((FleetManeuver.Duration / 2.0) / PhasingOrbitPeriod) * 360.0;
		Maneuver = FNovaManeuver(TransferB.StartDeltaV, CurrentPhase - ManeuverPhaseDelta, CurrentTime - FleetManeuver.Duration / 2.0,
			FleetManeuver.Duration, FleetManeuver.ThrustFactors);
		Trajectory->Add(Maneuver);
	}
	else
	{
		Maneuver = FNovaManeuver(TransferB.StartDeltaV, CurrentPhase, CurrentTime, FleetManeuver.Duration, FleetManeuver.ThrustFactors);
		Trajectory->Add(Maneuver);
	}

	// Second transfer
	Trajectory->Add(FNovaOrbit(
		FNovaOrbitGeometry(Parameters->Body, PhasingAltitude, DestinationAltitude, CurrentPhase, CurrentPhase + 180), CurrentTime));
	FleetManeuver = Fleet.AddManeuver(TransferB.EndDeltaV);
	CurrentPhase += 180;

	// Circularization burn after second transfer
	ManeuverPhaseDelta = (FleetManeuver.Duration / PhasingOrbitPeriod) * 360.0;
	Maneuver           = FNovaManeuver(TransferB.EndDeltaV, CurrentPhase - ManeuverPhaseDelta,
        CurrentTime + TransferB.Duration - FleetManeuver.Duration, FleetManeuver.Duration, FleetManeuver.ThrustFactors);
	Trajectory->Add(Maneuver);

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
	NLOG("InitialWaitingDuration %f, TotalTransferDuration %f", InitialWaitingDuration.AsMinutes(), TotalTransferDuration.AsMinutes());
	NLOG("DestinationPhaseChangeDuringTransfer %f, NewDestinationPhaseAfterTransfers %f, PhaseDelta %f",
		DestinationPhaseChangeDuringTransfer, NewDestinationPhaseAfterTransfers, PhaseDelta);
	NLOG("PhasingOrbitPeriod = %f, DestinationOrbitPeriod = %f", PhasingOrbitPeriod.AsMinutes(), DestinationOrbitPeriod.AsMinutes());
	NLOG("PhasingDuration = %f, PhasingAngle = %f", PhasingDuration.AsMinutes(), PhasingAngle);
	NLOG("FinalDestinationPhase = %f, FinalSpacecraftPhase = %f",
		FMath::IsFinite(FinalDestinationPhase) ? FMath::UnwindDegrees(FinalDestinationPhase) : 0,
		FMath::IsFinite(FinalSpacecraftPhase) ? FMath::UnwindDegrees(FinalSpacecraftPhase) : 0);
	NLOG("Trajectory->GetStartTime() = %f, Parameters->StartTime = %f",
		FMath::IsFinite(PhasingDuration.AsMinutes()) ? Trajectory->GetStartTime().AsMinutes() : 0, Parameters->StartTime.AsMinutes());
	NLOG("--------------------------------------------------------------------------------");
#endif

	if (FMath::IsFinite(PhasingDuration.AsMinutes()))
	{
		NCHECK(FMath::Abs(FMath::UnwindDegrees(FinalSpacecraftPhase) - FMath::UnwindDegrees(FinalDestinationPhase)) < 0.0001);
		NCHECK(FMath::Abs((Trajectory->GetStartTime() - Parameters->StartTime).AsSeconds()) < 1);
	}

#endif

	return Trajectory;
}

bool UNovaOrbitalSimulationComponent::IsOnStartedTrajectory(const FGuid& SpacecraftIdentifier) const
{
	const FNovaTrajectory* Trajectory = SpacecraftTrajectoryDatabase.Get(SpacecraftIdentifier);

	return Trajectory && GetCurrentTime() >= Trajectory->GetFirstManeuverStartTime() && GetCurrentTime() <= Trajectory->GetArrivalTime();
}

bool UNovaOrbitalSimulationComponent::CanCommitTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory, FText* Help) const
{
	const ANovaGameState* GameState = GetOwner<ANovaGameState>();

	if (GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		if (Help)
		{
			*Help = LOCTEXT("CannotCommitTrajectoryHost", "Only the hosting player can commit to a trajectory");
		}
		return false;
	}
	else if (!Trajectory.IsValid())
	{
		if (Help)
		{
			*Help = LOCTEXT("CannotCommitTrajectoryValidity", "The selected trajectory is not valid");
		}
		return false;
	}
	else if (Trajectory->Maneuvers[0].Time <= GetCurrentTime())
	{
		if (Help)
		{
			*Help = LOCTEXT("CannotCommitTrajectoryTime", "The selected trajectory starts in the past");
		}
		return false;
	}
	else if (GameState->IsAnySpacecraftDocked())
	{
		if (Help)
		{
			*Help = LOCTEXT("CannotCommitTrajectoryDocked", "A spacecraft is still docked");
		}
		return false;
	}

	return true;
}

void UNovaOrbitalSimulationComponent::CommitTrajectory(
	const TArray<FGuid>& SpacecraftIdentifiers, const TSharedPtr<FNovaTrajectory>& Trajectory)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
	NCHECK(Trajectory.IsValid() && Trajectory->IsValid());

	NLOG("UNovaOrbitalSimulationComponent::CommitTrajectory for %d spacecraft", SpacecraftIdentifiers.Num());

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
    Simple trajectory & orbiting getters
----------------------------------------------------*/

int32 UNovaOrbitalSimulationComponent::GetPlayerSpacecraftIndex(const FGuid& Identifier) const
{
	int32 Index = 0;

	const ANovaGameState* GameState = GetOwner<ANovaGameState>();
	for (FGuid SpacecraftIdentifier : GameState->GetPlayerSpacecraftIdentifiers())
	{
		if (SpacecraftIdentifier == Identifier)
		{
			return Index;
		}
		else
		{
			Index++;
		}
	}

	return INDEX_NONE;
}

/*----------------------------------------------------
    Trajectory & orbiting getters
----------------------------------------------------*/

UNovaOrbitalSimulationComponent* UNovaOrbitalSimulationComponent::Get(const UObject* Outer)
{
	if (IsValid(Outer) && IsValid(Outer->GetWorld()))
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
				Identifiers.Num(), GetCurrentTime().AsMinutes());
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
