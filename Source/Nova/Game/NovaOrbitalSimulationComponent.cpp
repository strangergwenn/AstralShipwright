// Nova project - Gwennaël Arbona

#include "NovaOrbitalSimulationComponent.h"
#include "NovaArea.h"
#include "NovaAssetCatalog.h"
#include "NovaGameInstance.h"
#include "NovaGameWorld.h"
#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

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
	const class UNovaArea* Source, const class UNovaArea* Destination)
{
	TSharedPtr<FNovaTrajectory> Trajectory = MakeShared<FNovaTrajectory>(FNovaOrbit(Source->Altitude, Source->Phase));

	float PhasingAltitude = 200;
	float PhasingAngle    = 90;
	float InitialAngle    = Source->Phase;

	Trajectory->TransferOrbits.Add(FNovaOrbit(Source->Altitude, PhasingAltitude, InitialAngle, InitialAngle + 180));
	Trajectory->TransferOrbits.Add(FNovaOrbit(PhasingAltitude, PhasingAltitude, InitialAngle + 180, InitialAngle + 180 + PhasingAngle));
	Trajectory->TransferOrbits.Add(
		FNovaOrbit(PhasingAltitude, Destination->Altitude, InitialAngle + 180 + PhasingAngle, InitialAngle + 360 + PhasingAngle));

	Trajectory->FinalOrbit = FNovaOrbit(Destination->Altitude, Destination->Phase);

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
