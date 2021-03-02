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
}

TSharedPtr<FNovaTrajectory> UNovaOrbitalSimulationComponent::ComputeTrajectory(
	const class UNovaArea* Source, const class UNovaArea* Destination)
{
	TSharedPtr<FNovaTrajectory> Trajectory = MakeShared<FNovaTrajectory>(FNovaOrbit(Source->Altitude, Source->Phase));

	return Trajectory;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaOrbitalSimulationComponent::UpdateTrajectories()
{
	ANovaGameWorld* GameWorld = GetOwner<ANovaGameWorld>();

	AreaTrajectories.Empty();

	for (const UNovaArea* Area : Areas)
	{
		AreaTrajectories.Add(Area, FNovaOrbit(Area->Altitude, Area->Phase));
	}
}
