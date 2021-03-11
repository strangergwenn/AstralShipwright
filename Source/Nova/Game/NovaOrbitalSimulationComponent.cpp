// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationComponent.h"
#include "NovaArea.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaGameWorld.h"

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
	SetIsReplicatedByDefault(false);
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

	UpdateTrajectories();
	UpdatePositions();
}

TSharedPtr<FNovaTrajectory> UNovaOrbitalSimulationComponent::ComputeTrajectory(
	const class UNovaArea* Source, const class UNovaArea* Destination, float PhasingAltitude)
{
	// Basic parameters
	const UNovaPlanet* Planet = Source->Planet;
	const double       µ      = Planet->GetGravitationalParameter();
	const double       R1     = Planet->GetRadius(Source->Altitude);
	const double       R2     = Planet->GetRadius(PhasingAltitude);
	const double       R3     = Planet->GetRadius(Destination->Altitude);

	// Compute both Hohmann transfers as well as the orbital periods
	const FNovaHohmannTransfer TransferA              = ComputeHohmannTransfer(µ, R1, R2);
	const FNovaHohmannTransfer TransferB              = ComputeHohmannTransfer(µ, R2, R3);
	const double               PhasingOrbitPeriod     = GetCircularOrbitPeriod(µ, R2);
	const double               DestinationOrbitPeriod = GetCircularOrbitPeriod(µ, R3);

	// Compute the new destination parameters after both transfers, ignoring the phasing orbit
	const double TotalTransferDuration                = TransferA.Duration + TransferB.Duration;
	const double DestinationPhaseChangeDuringTransfer = (DestinationOrbitPeriod / TotalTransferDuration) * 360.0;
	const double NewDestinationPhaseAfterTransfers    = fmod(Destination->Phase + DestinationPhaseChangeDuringTransfer, 360.0);
	double       PhaseDelta                           = fmod(NewDestinationPhaseAfterTransfers - Source->Phase + 360.0, 360.0);
	if (PhasingOrbitPeriod > DestinationOrbitPeriod)
	{
		PhaseDelta = PhaseDelta - 360.0;
	}

	// Compute the time spent waiting
	const double PhasingDuration = PhaseDelta / (360.0 * (1.0 / PhasingOrbitPeriod - 1.0 / DestinationOrbitPeriod));
	float        PhasingAngle    = (PhasingDuration / PhasingOrbitPeriod) * 360.0;

#if 0

	float DestinationPhasingAngle = (PhasingDuration / DestinationOrbitPeriod) * 360.0;
	float FinalDestinationPhase   = fmod(NewDestinationPhaseAfterTransfers + DestinationPhasingAngle, 360.0);
	float FinalSpacecraftPhase    = fmod(Source->Phase + PhasingAngle, 360.0);

	NLOG("UNovaOrbitalSimulationComponent::ComputeTrajectory : (%f, %f) --> (%f, %f)", Source->Altitude, Source->Phase,
		Destination->Altitude, Destination->Phase);
	NLOG("Transfer A : DVS %f, DVE %f, DV %f, T %f", TransferA.StartDeltaV, TransferA.EndDeltaV, TransferA.TotalDeltaV, TransferA.Duration);
	NLOG("Transfer B : DVS %f, DVE %f, DV %f, T %f", TransferB.StartDeltaV, TransferB.EndDeltaV, TransferB.TotalDeltaV, TransferB.Duration);
	NLOG("DestinationPhaseChangeDuringTransfer %f, NewDestinationPhaseAfterTransfers %f, PhaseDelta %f",
		DestinationPhaseChangeDuringTransfer, NewDestinationPhaseAfterTransfers, PhaseDelta);
	NLOG("PhasingOrbitPeriod = %f, DestinationOrbitPeriod = %f", PhasingOrbitPeriod, DestinationOrbitPeriod);
	NLOG("PhasingDuration = %f, PhasingAngle = %f", PhasingDuration, PhasingAngle);
	NLOG("FinalDestinationPhase = %f, FinalSpacecraftPhase = %f", FinalDestinationPhase, FinalSpacecraftPhase);

#endif

	// Initial orbit
	TSharedPtr<FNovaTrajectory> Trajectory           = MakeShared<FNovaTrajectory>(FNovaOrbit(Source->Altitude, Source->Phase));
	auto                        AddManeuverIfNotNull = [&Trajectory](const FNovaManeuver& Maneuver)
	{
		if (Maneuver.DeltaV != 0)
		{
			Trajectory->Maneuvers.Add(Maneuver);
		}
	};

	// First transfer
	AddManeuverIfNotNull(FNovaManeuver(TransferA.StartDeltaV, 0, Source->Phase));
	Trajectory->TransferOrbits.Add(FNovaOrbit(Source->Altitude, PhasingAltitude, Source->Phase, Source->Phase + 180));
	AddManeuverIfNotNull(FNovaManeuver(TransferA.EndDeltaV, TransferA.Duration, Source->Phase + 180));

	// Phasing orbit
	Trajectory->TransferOrbits.Add(FNovaOrbit(PhasingAltitude, PhasingAltitude, Source->Phase + 180, Source->Phase + 180 + PhasingAngle));

	// Second transfer
	AddManeuverIfNotNull(FNovaManeuver(TransferB.StartDeltaV, TransferA.Duration + PhasingDuration, Source->Phase + 180 + PhasingAngle));
	Trajectory->TransferOrbits.Add(
		FNovaOrbit(PhasingAltitude, Destination->Altitude, Source->Phase + 180 + PhasingAngle, Source->Phase + 360 + PhasingAngle));
	AddManeuverIfNotNull(
		FNovaManeuver(TransferB.EndDeltaV, TransferA.Duration + PhasingDuration + TransferB.Duration, Source->Phase + 360 + PhasingAngle));

	// Final orbit
	Trajectory->FinalOrbit = FNovaOrbit(Destination->Altitude, Destination->Phase);

	// Metadata
	Trajectory->TotalTransferDuration = TransferA.Duration + PhasingDuration + TransferB.Duration;
	Trajectory->TotalDeltaV           = TransferA.TotalDeltaV + TransferB.TotalDeltaV;

	return Trajectory;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::UpdateTrajectories()
{
	ANovaGameWorld* GameWorld = GetOwner<ANovaGameWorld>();

	Trajectories.Empty();

	for (const UNovaArea* Area : Areas)
	{
		Trajectories.Add(Area, FNovaOrbit(Area->Altitude, Area->Phase));
	}
}

void UNovaOrbitalSimulationComponent::UpdatePositions()
{
	ANovaGameWorld* GameWorld = GetOwner<ANovaGameWorld>();

	Positions.Empty();

	for (const UNovaArea* Area : Areas)
	{
		Positions.Add(Area, Area->Phase);
	}
}

#undef LOCTEXT_NAMESPACE
