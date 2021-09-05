// Nova project - Gwennaël Arbona

#include "NovaPlanetarium.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationTypes.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"

#include "Nova/Nova.h"

#include "EngineUtils.h"
#include "Engine.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceConstant.h"

#define CELESTIAL_SCALE 0.001

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaPlanetarium::ANovaPlanetarium() : Super()
{
	// Settings
	PrimaryActorTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Implementation
----------------------------------------------------*/

void ANovaPlanetarium::BeginPlay()
{
	Super::BeginPlay();
	NLOG("ANovaPlanetarium::BeginPlay");

	const UNovaGameInstance* GameInstance = GetWorld()->GetGameInstance<UNovaGameInstance>();
	NCHECK(GameInstance);
	const UNovaAssetManager* AssetManager = GameInstance->GetAssetManager();
	NCHECK(AssetManager);

	// Find all owned mesh components & all celestial bodies
	TArray<UActorComponent*> MeshComponents;
	GetComponents(UStaticMeshComponent::StaticClass(), MeshComponents);
	TArray<const UNovaCelestialBody*> CelestialBodies = AssetManager->GetAssets<UNovaCelestialBody>();

	// For each known celestial body, map it to a component
	for (const UNovaCelestialBody* Body : CelestialBodies)
	{
		if (Body->Body == nullptr)
		{
			RootBody = Body;
		}

		for (UActorComponent* Component : MeshComponents)
		{
			if (Component->GetName() == Body->PlanetariumName.ToString())
			{
				NLOG("ANovaPlanetarium::BeginPlay : found mesh for '%s'", *Body->PlanetariumName.ToString());
				CelestialToComponent.Add(Body, Cast<UStaticMeshComponent>(Component));
			}
		}
	}

	NCHECK(CelestialToComponent.Num() == CelestialBodies.Num());
}

void ANovaPlanetarium::Tick(float DeltaSeconds)
{
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();
	NCHECK(PlayerLocation);

	for (const TPair<const UNovaCelestialBody*, UStaticMeshComponent*> BodyAndComponent : CelestialToComponent)
	{
		const UNovaCelestialBody* Body      = BodyAndComponent.Key;
		UStaticMeshComponent*     Component = BodyAndComponent.Value;

		if (Body != RootBody)
		{
			Component->SetWorldLocation(
				CELESTIAL_SCALE * FVector((Body->Radius + PlayerLocation->Geometry.StartAltitude) * 1000 * 100, 0, 0));
			Component->SetWorldScale3D(CELESTIAL_SCALE * 2.0 * Body->Radius * 1000 * FVector(1, 1, 1));
		}

		else
		{
			Component->SetWorldLocation(
				-CELESTIAL_SCALE * FVector((Body->Radius + PlayerLocation->Geometry.Body->Altitude.GetValue()) * 1000 * 100, 0, 0));
			Component->SetWorldScale3D(CELESTIAL_SCALE * 2.0 * Body->Radius * 1000 * FVector(1, 1, 1));
		}
	}
}
