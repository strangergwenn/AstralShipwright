// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationComponent.h"
#include "NovaArea.h"
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
    Constructor
----------------------------------------------------*/

UNovaOrbitalSimulationComponent::UNovaOrbitalSimulationComponent() : Super()
{
	// Settings
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    Gameplay
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

	// Run processes
	ProcessAreas();
	ProcessSpacecraftTrajectories();
}

FNovaTrajectoryParameters UNovaOrbitalSimulationComponent::PrepareTrajectory(
	const UNovaArea* Source, const UNovaArea* Destination, double DeltaTime) const
{
	FNovaTrajectoryParameters Parameters;

	NCHECK(Source != nullptr && Destination != nullptr);
	NCHECK(Source->Planet != nullptr && Destination->Planet != nullptr);
	NCHECK(Source->Planet == Destination->Planet);

	// Get basic parameters
	Parameters.StartTime           = GetCurrentTime() + DeltaTime;
	Parameters.SourcePhase         = GetAreaPhase(Source, DeltaTime);
	Parameters.DestinationPhase    = GetAreaPhase(Destination, DeltaTime);
	Parameters.SourceAltitude      = Source->Altitude;
	Parameters.DestinationAltitude = Destination->Altitude;

	// Get orbital parameters
	Parameters.Planet = Source->Planet;
	Parameters.µ      = Source->Planet->GetGravitationalParameter();

	return Parameters;
}

TSharedPtr<FNovaTrajectory> UNovaOrbitalSimulationComponent::ComputeTrajectory(
	const FNovaTrajectoryParameters& Parameters, float PhasingAltitude)
{
	// Get phase and altitude
	const double& StartTime           = Parameters.StartTime;
	const double& SourceAltitude      = Parameters.SourceAltitude;
	const double& SourcePhase         = Parameters.SourcePhase;
	const double& DestinationAltitude = Parameters.DestinationAltitude;
	const double& DestinationPhase    = Parameters.DestinationPhase;

	// Get orbital parameters
	const double R1 = Parameters.Planet->GetRadius(SourceAltitude);
	const double R2 = Parameters.Planet->GetRadius(PhasingAltitude);
	const double R3 = Parameters.Planet->GetRadius(DestinationAltitude);

	// Compute both Hohmann transfers as well as the orbital periods
	const FNovaHohmannTransfer& TransferA              = ComputeHohmannTransfer(Parameters.µ, R1, R2);
	const FNovaHohmannTransfer& TransferB              = ComputeHohmannTransfer(Parameters.µ, R2, R3);
	const double&               PhasingOrbitPeriod     = GetCircularOrbitPeriod(Parameters.µ, R2);
	const double&               DestinationOrbitPeriod = GetCircularOrbitPeriod(Parameters.µ, R3);

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
	float DestinationPhasingAngle = (PhasingDuration / DestinationOrbitPeriod) * 360.0;
	float FinalDestinationPhase   = fmod(NewDestinationPhaseAfterTransfers + DestinationPhasingAngle, 360.0);
	float FinalSpacecraftPhase    = fmod(SourcePhase + PhasingAngle, 360.0);
	NCHECK(FinalSpacecraftPhase == FinalDestinationPhase);

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

	// Helpers
	auto AddManeuverIfNotNull = [&Trajectory](const FNovaManeuver& Maneuver)
	{
		if (Maneuver.DeltaV != 0)
		{
			Trajectory->Maneuvers.Add(Maneuver);
		}
	};
	auto AddTransferIfNotNull = [&Trajectory](const FNovaOrbit& Orbit)
	{
		if (Orbit.StartPhase != Orbit.EndPhase)
		{
			Trajectory->TransferOrbits.Add(Orbit);
		}
	};

	// First transfer
	AddManeuverIfNotNull(FNovaManeuver(TransferA.StartDeltaV, StartTime, SourcePhase));
	AddTransferIfNotNull(FNovaOrbit(SourceAltitude, PhasingAltitude, SourcePhase, SourcePhase + 180));
	AddManeuverIfNotNull(FNovaManeuver(TransferA.EndDeltaV, StartTime + TransferA.Duration, SourcePhase + 180));

	// Phasing orbit
	AddTransferIfNotNull(FNovaOrbit(PhasingAltitude, PhasingAltitude, SourcePhase + 180, SourcePhase + 180 + PhasingAngle));

	// Second transfer
	AddManeuverIfNotNull(
		FNovaManeuver(TransferB.StartDeltaV, StartTime + TransferA.Duration + PhasingDuration, SourcePhase + 180 + PhasingAngle));
	AddTransferIfNotNull(
		FNovaOrbit(PhasingAltitude, DestinationAltitude, SourcePhase + 180 + PhasingAngle, SourcePhase + 360 + PhasingAngle));
	AddManeuverIfNotNull(FNovaManeuver(
		TransferB.EndDeltaV, StartTime + TransferA.Duration + PhasingDuration + TransferB.Duration, SourcePhase + 360 + PhasingAngle));

	// Metadata
	Trajectory->TotalTransferDuration = TransferA.Duration + PhasingDuration + TransferB.Duration;
	Trajectory->TotalDeltaV           = TransferA.TotalDeltaV + TransferB.TotalDeltaV;

	return Trajectory;
}

bool UNovaOrbitalSimulationComponent::CanCommitTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory) const
{
	return GetOwner()->GetLocalRole() == ROLE_Authority && Trajectory.IsValid() && Trajectory->Maneuvers.Num() > 0 &&
		   Trajectory->Maneuvers[0].Time > GetCurrentTime();
}

void UNovaOrbitalSimulationComponent::CommitTrajectory(
	const TSharedPtr<FNovaTrajectory>& Trajectory, const TArray<FGuid>& SpacecraftIdentifiers)
{
	NCHECK(CanCommitTrajectory(Trajectory));

	NLOG("UNovaOrbitalSimulationComponent::CommitTrajectory for %d spacecraft", SpacecraftIdentifiers.Num());

	// Move spacecraft to the trajectory
	OrbitDatabase.Remove(SpacecraftIdentifiers);
	TrajectoryDatabase.Add(Trajectory, SpacecraftIdentifiers);
}

const FNovaTrajectory* UNovaOrbitalSimulationComponent::GetCommittedPlayerTrajectory() const
{
	for (const ANovaPlayerState* PlayerState : TActorRange<ANovaPlayerState>(GetWorld()))
	{
		if (IsValid(PlayerState))
		{
			return TrajectoryDatabase.GetTrajectory(PlayerState->GetSpacecraftIdentifier());
		}
	}

	return nullptr;
}

void UNovaOrbitalSimulationComponent::CompleteTrajectory(const TArray<FGuid>& SpacecraftIdentifiers)
{
	NLOG("UNovaOrbitalSimulationComponent::CompleteTrajectory for %d spacecraft", SpacecraftIdentifiers.Num());

	const FNovaTimedOrbit CommonFinalOrbit;
	bool                  FoundCommonOrbit = false;

	// Compute the final orbit and ensure all spacecraft are going there
	for (const FGuid& Identifier : SpacecraftIdentifiers)
	{
		const FNovaTrajectory* Trajectory = TrajectoryDatabase.GetTrajectory(Identifier);
		if (Trajectory)
		{
			const FNovaTimedOrbit FinalOrbit = Trajectory->GetFinalOrbit();
			NCHECK(!FoundCommonOrbit || FinalOrbit == CommonFinalOrbit);
			FoundCommonOrbit = true;
		}
	}

	// Commit the change
	TrajectoryDatabase.Remove(SpacecraftIdentifiers);
	OrbitDatabase.Add(MakeShared<FNovaTimedOrbit>(CommonFinalOrbit), SpacecraftIdentifiers);
}

double UNovaOrbitalSimulationComponent::GetCurrentTime() const
{
	// TODO : shared time for multiplayer
	return GetWorld()->GetTimeSeconds() / 60.0;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::ProcessAreas()
{
	ANovaGameWorld* GameWorld = GetOwner<ANovaGameWorld>();

	for (const UNovaArea* Area : Areas)
	{
		const UNovaPlanet* Planet = Area->Planet;
		NCHECK(Planet);

		// Update the position
		double CurrentPhase = GetAreaPhase(Area);

		// Add or update the current orbit and position
		FNovaOrbitalLocation* Entry = AreasOrbitalLocation.Find(Area);
		if (Entry)
		{
			Entry->Phase = CurrentPhase;
		}
		else
		{
			AreasOrbitalLocation.Add(Area, FNovaOrbitalLocation(FNovaOrbit(Area->Altitude, Area->Phase), CurrentPhase));
		}
	}
}

void UNovaOrbitalSimulationComponent::ProcessSpacecraftOrbits()
{
	for (const FNovaOrbitDatabaseEntry& DatabaseEntry : OrbitDatabase.GetOrbits())
	{
	}
}

void UNovaOrbitalSimulationComponent::ProcessSpacecraftTrajectories()
{
	for (const FNovaTrajectoryDatabaseEntry& DatabaseEntry : TrajectoryDatabase.GetTrajectories())
	{
	}
}

double UNovaOrbitalSimulationComponent::GetAreaPhase(const UNovaArea* Area, double DeltaTime) const
{
	const double OrbitalPeriod = GetCircularOrbitPeriod(Area->Planet->GetGravitationalParameter(), Area->Planet->GetRadius(Area->Altitude));
	const double CurrentTime   = GetCurrentTime() + DeltaTime;
	return FMath::Fmod(Area->Phase + (CurrentTime / OrbitalPeriod) * 360, 360.0);
}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaOrbitalSimulationComponent, OrbitDatabase);
	DOREPLIFETIME(UNovaOrbitalSimulationComponent, TrajectoryDatabase);
}

#undef LOCTEXT_NAMESPACE
