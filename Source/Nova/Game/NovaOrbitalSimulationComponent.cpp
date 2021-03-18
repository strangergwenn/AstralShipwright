// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationComponent.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaGameWorld.h"

#include "Nova/Player/NovaPlayerState.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"
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
	FNovaHohmannTransfer(const double GravitationalParameter, const double SourceRadius, const double DestinationRadius)
	{
		StartDeltaV =
			abs(sqrt(GravitationalParameter / SourceRadius) * (sqrt((2.0 * DestinationRadius) / (SourceRadius + DestinationRadius)) - 1.0));
		EndDeltaV =
			abs(sqrt(GravitationalParameter / DestinationRadius) * (1.0 - sqrt((2.0 * SourceRadius) / (SourceRadius + DestinationRadius))));

		TotalDeltaV = StartDeltaV + EndDeltaV;

		Duration = PI * sqrt(pow(SourceRadius + DestinationRadius, 3.0) / (8.0 * GravitationalParameter)) / 60;
	}

	double StartDeltaV;
	double EndDeltaV;
	double TotalDeltaV;
	double Duration;
};

/** Trajectory computation parameters */
struct FNovaTrajectoryParameters
{
	double StartTime;

	double SourceAltitude;
	double SourcePhase;
	double DestinationAltitude;
	double DestinationPhase;

	const UNovaPlanet* Planet;
	double             µ;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaOrbitalSimulationComponent::UNovaOrbitalSimulationComponent() : Super()
{
	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// Defaults
	OrbitGarbageCollectionDelay = 1.0f;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::BeginPlay()
{
	Super::BeginPlay();

	Areas = GetOwner()->GetGameInstance<UNovaGameInstance>()->GetCatalog()->GetAssets<UNovaArea>();
}

void UNovaOrbitalSimulationComponent::TickComponent(
	float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get a player state
	CurrentPlayerState = nullptr;
	for (const ANovaPlayerState* PlayerState : TActorRange<ANovaPlayerState>(GetWorld()))
	{
		if (IsValid(PlayerState))
		{
			CurrentPlayerState = PlayerState;
			break;
		}
	}

	// Run processes
	ProcessOrbitCleanup();
	ProcessAreas();
	ProcessSpacecraftOrbits();
	ProcessSpacecraftTrajectories();
}

/*----------------------------------------------------
    Trajectory & orbiting interface
----------------------------------------------------*/

TSharedPtr<FNovaTrajectoryParameters> UNovaOrbitalSimulationComponent::PrepareTrajectory(
	const UNovaArea* Source, const UNovaArea* Destination, double DeltaTime) const
{
	TSharedPtr<FNovaTrajectoryParameters> Parameters = MakeShared<FNovaTrajectoryParameters>();

	NCHECK(Source != nullptr && Destination != nullptr);
	NCHECK(Source->Planet != nullptr && Destination->Planet != nullptr);
	NCHECK(Source->Planet == Destination->Planet);

	auto GetAreaPhase = [](const UNovaArea* Area, double CurrentTime)
	{
		const double OrbitalPeriod = GetOrbitalPeriod(Area->Planet->GetGravitationalParameter(), Area->Planet->GetRadius(Area->Altitude));
		return FMath::Fmod(Area->Phase + (CurrentTime / OrbitalPeriod) * 360, 360.0);
	};

	// Get basic parameters
	Parameters->StartTime           = GetCurrentTime() + DeltaTime;
	Parameters->SourcePhase         = GetAreaPhase(Source, GetCurrentTime() + DeltaTime);
	Parameters->DestinationPhase    = GetAreaPhase(Destination, GetCurrentTime() + DeltaTime);
	Parameters->SourceAltitude      = Source->Altitude;
	Parameters->DestinationAltitude = Destination->Altitude;

	// Get orbital parameters
	Parameters->Planet = Source->Planet;
	Parameters->µ      = Source->Planet->GetGravitationalParameter();

	return Parameters;
}

TSharedPtr<FNovaTrajectory> UNovaOrbitalSimulationComponent::ComputeTrajectory(
	const TSharedPtr<FNovaTrajectoryParameters>& Parameters, float PhasingAltitude)
{
	// Get phase and altitude
	const double& StartTime           = Parameters->StartTime;
	const double& SourceAltitude      = Parameters->SourceAltitude;
	const double& SourcePhase         = Parameters->SourcePhase;
	const double& DestinationAltitude = Parameters->DestinationAltitude;
	const double& DestinationPhase    = Parameters->DestinationPhase;

	// Get orbital parameters
	const double R1 = Parameters->Planet->GetRadius(SourceAltitude);
	const double R2 = Parameters->Planet->GetRadius(PhasingAltitude);
	const double R3 = Parameters->Planet->GetRadius(DestinationAltitude);

	// Compute both Hohmann transfers as well as the orbital periods
	const FNovaHohmannTransfer TransferA(Parameters->µ, R1, R2);
	const FNovaHohmannTransfer TransferB(Parameters->µ, R2, R3);
	const double&              PhasingOrbitPeriod     = GetOrbitalPeriod(Parameters->µ, R2);
	const double&              DestinationOrbitPeriod = GetOrbitalPeriod(Parameters->µ, R3);

	// Compute the new destination parameters after both transfers, ignoring the phasing orbit
	const double TotalTransferDuration                = TransferA.Duration + TransferB.Duration;
	const double DestinationPhaseChangeDuringTransfer = (DestinationOrbitPeriod / TotalTransferDuration) * 360.0;
	const double NewDestinationPhaseAfterTransfers    = fmod(DestinationPhase + DestinationPhaseChangeDuringTransfer, 360.0);
	double       PhaseDelta                           = fmod(NewDestinationPhaseAfterTransfers - SourcePhase + 360.0, 360.0);
	if (PhasingOrbitPeriod > DestinationOrbitPeriod)
	{
		PhaseDelta = PhaseDelta - 360.0;
	}

	// Compute the time spent waiting
	const double PhasingDuration = PhaseDelta / (360.0 * (1.0 / PhasingOrbitPeriod - 1.0 / DestinationOrbitPeriod));
	const double PhasingAngle    = (PhasingDuration / PhasingOrbitPeriod) * 360.0;

#if WITH_EDITOR

	// Confirm the final spacecraft phase matches the destination's
	const double DestinationPhasingAngle = (PhasingDuration / DestinationOrbitPeriod) * 360.0;
	const double FinalDestinationPhase   = fmod(NewDestinationPhaseAfterTransfers + DestinationPhasingAngle, 360.0);
	const double FinalSpacecraftPhase    = fmod(SourcePhase + PhasingAngle, 360.0);
	NCHECK(FMath::Abs(FinalSpacecraftPhase - FinalDestinationPhase) < SMALL_NUMBER);

#endif

#if 0

	NLOG("UNovaOrbitalSimulationComponent::ComputeTrajectory : (%f, %f) --> (%f, %f)", SourceAltitude, SourcePhase, DestinationAltitude,
		DestinationPhase);
	NLOG("Transfer A : DVS %f, DVE %f, DV %f, T %f", TransferA.StartDeltaV, TransferA.EndDeltaV, TransferA.TotalDeltaV, TransferA.Duration);
	NLOG("Transfer B : DVS %f, DVE %f, DV %f, T %f", TransferB.StartDeltaV, TransferB.EndDeltaV, TransferB.TotalDeltaV, TransferB.Duration);
	NLOG("DestinationPhaseChangeDuringTransfer %f, NewDestinationPhaseAfterTransfers %f, PhaseDelta %f",
		DestinationPhaseChangeDuringTransfer, NewDestinationPhaseAfterTransfers, PhaseDelta);
	NLOG("PhasingOrbitPeriod = %f, DestinationOrbitPeriod = %f", PhasingOrbitPeriod, DestinationOrbitPeriod);
	NLOG("PhasingDuration = %f, PhasingAngle = %f", PhasingDuration, PhasingAngle);
	NLOG("FinalDestinationPhase = %f, FinalSpacecraftPhase = %f", FinalDestinationPhase, FinalSpacecraftPhase);

#endif

	TSharedPtr<FNovaTrajectory> Trajectory = MakeShared<FNovaTrajectory>();
	Trajectory->StartTime                  = StartTime;

	// Helpers
	auto AddManeuverIfNotNull = [&Trajectory](const FNovaManeuver& Maneuver)
	{
		if (Maneuver.DeltaV != 0)
		{
			Trajectory->Maneuvers.Add(Maneuver);
		}
	};
	auto AddTransferIfNotNull = [&Trajectory](const FNovaOrbitGeometry& Geometry)
	{
		if (Geometry.StartPhase != Geometry.EndPhase)
		{
			Trajectory->TransferOrbits.Add(Geometry);
		}
	};

	// First transfer
	AddManeuverIfNotNull(FNovaManeuver(TransferA.StartDeltaV, StartTime, SourcePhase));
	AddTransferIfNotNull(FNovaOrbitGeometry(Parameters->Planet, SourceAltitude, PhasingAltitude, SourcePhase, SourcePhase + 180));
	AddManeuverIfNotNull(FNovaManeuver(TransferA.EndDeltaV, StartTime + TransferA.Duration, SourcePhase + 180));

	// Phasing orbit
	AddTransferIfNotNull(
		FNovaOrbitGeometry(Parameters->Planet, PhasingAltitude, PhasingAltitude, SourcePhase + 180, SourcePhase + 180 + PhasingAngle));

	// Second transfer
	AddManeuverIfNotNull(
		FNovaManeuver(TransferB.StartDeltaV, StartTime + TransferA.Duration + PhasingDuration, SourcePhase + 180 + PhasingAngle));
	AddTransferIfNotNull(FNovaOrbitGeometry(
		Parameters->Planet, PhasingAltitude, DestinationAltitude, SourcePhase + 180 + PhasingAngle, SourcePhase + 360 + PhasingAngle));
	AddManeuverIfNotNull(FNovaManeuver(
		TransferB.EndDeltaV, StartTime + TransferA.Duration + PhasingDuration + TransferB.Duration, SourcePhase + 360 + PhasingAngle));

	// Metadata
	Trajectory->TotalTransferDuration = TransferA.Duration + PhasingDuration + TransferB.Duration;
	Trajectory->TotalDeltaV           = TransferA.TotalDeltaV + TransferB.TotalDeltaV;

	return Trajectory;
}

bool UNovaOrbitalSimulationComponent::IsOnTrajectory(const FGuid& SpacecraftIdentifier) const
{
	const FNovaTrajectory* Trajectory = SpacecraftTrajectoryDatabase.Get(SpacecraftIdentifier);

	return Trajectory && GetCurrentTime() >= Trajectory->StartTime && GetCurrentTime() <= Trajectory->GetArrivalTime();
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
	SpacecraftTrajectoryDatabase.AddOrUpdate(SpacecraftIdentifiers, Trajectory);
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
	SpacecraftOrbitDatabase.AddOrUpdate(SpacecraftIdentifiers, Orbit);
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
	ANovaGameWorld* GameWorld = ANovaGameWorld::Get(Outer);
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* SimulationComponent = GameWorld->GetOrbitalSimulation();
	NCHECK(SimulationComponent);

	return SimulationComponent;
}

TPair<const UNovaArea*, float> UNovaOrbitalSimulationComponent::GetClosestAreaAndDistance(const FNovaOrbitalLocation& Location) const
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

FGuid UNovaOrbitalSimulationComponent::GetPlayerSpacecraftIdentifier() const
{
	return CurrentPlayerState ? CurrentPlayerState->GetSpacecraftIdentifier() : FGuid(0, 0, 0, 0);
}

const FNovaOrbit* UNovaOrbitalSimulationComponent::GetPlayerOrbit() const
{
	if (IsValid(CurrentPlayerState))
	{
		return GetSpacecraftOrbit(CurrentPlayerState->GetSpacecraftIdentifier());
	}
	else
	{
		return nullptr;
	}
}

const FNovaTrajectory* UNovaOrbitalSimulationComponent::GetPlayerTrajectory() const
{
	if (IsValid(CurrentPlayerState))
	{
		return GetSpacecraftTrajectory(CurrentPlayerState->GetSpacecraftIdentifier());
	}
	else
	{
		return nullptr;
	}
}

const FNovaOrbitalLocation* UNovaOrbitalSimulationComponent::GetPlayerLocation() const
{
	if (IsValid(CurrentPlayerState))
	{
		return GetSpacecraftLocation(CurrentPlayerState->GetSpacecraftIdentifier());
	}
	else
	{
		return nullptr;
	}
}

double UNovaOrbitalSimulationComponent::GetCurrentTime() const
{
	return Cast<ANovaGameWorld>(GetOwner())->GetCurrentTime();
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
			if (GetCurrentTime() >= DatabaseEntry.Trajectory.StartTime + OrbitGarbageCollectionDelay)
			{
				SpacecraftOrbitDatabase.Remove(DatabaseEntry.Identifiers);
			}
		}
	}
}

void UNovaOrbitalSimulationComponent::ProcessAreas()
{
	ANovaGameWorld* GameWorld = GetOwner<ANovaGameWorld>();

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
		if (GetCurrentTime() >= DatabaseEntry.Trajectory.StartTime)
		{
			FNovaOrbitalLocation NewLocation = DatabaseEntry.Trajectory.GetCurrentLocation(GetCurrentTime());

#if 0
			NLOG("UNovaOrbitalSimulationComponent::ProcessSpacecraftTrajectories : %s has phase %f, with sphase %f, ephase %f",
				*DatabaseEntry.Identifiers[0].ToString(), NewLocation.Phase, NewLocation.Geometry.StartPhase,
				NewLocation.Geometry.EndPhase);
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

			// Complete the trajectory on arrival
			if (GetCurrentTime() > DatabaseEntry.Trajectory.GetArrivalTime())
			{
				CompletedTrajectories.Add(DatabaseEntry.Identifiers);
			}
		}
	}

	// Complete trajectories
	for (const TArray<FGuid>& Identifiers : CompletedTrajectories)
	{
		CompleteTrajectory(Identifiers);
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
