// Astral Shipwright - Gwennaël Arbona

#include "NovaPlanetarium.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationTypes.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"

#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"

#include "Materials/MaterialInstanceConstant.h"
#include "EngineUtils.h"
#include "Engine.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaPlanetarium::ANovaPlanetarium() : Super(), CurrentSunSkyAngle(0)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	// Create sunlight
	SunRotator = CreateDefaultSubobject<USceneComponent>("SunRotator");
	SunRotator->SetupAttachment(RootComponent);

	// Create sunlight
	Sunlight = CreateDefaultSubobject<UDirectionalLightComponent>("Sunlight");
	Sunlight->SetupAttachment(SunRotator);
	Sunlight->bAtmosphereSunLight = true;

	// Create skylight
	Skylight = CreateDefaultSubobject<USkyLightComponent>("Skylight");
	Skylight->SetupAttachment(RootComponent);
	Skylight->SourceType       = ESkyLightSourceType::SLS_CapturedScene;
	Skylight->bRealTimeCapture = true;

	// Create atmosphere
	Skybox = CreateDefaultSubobject<UStaticMeshComponent>("Skybox");
	Skybox->SetupAttachment(RootComponent);
	Skybox->SetWorldScale3D(1000000000 * FVector(1, 1, 1));
	Skybox->SetCastShadow(false);
	Skybox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create atmosphere
	Atmosphere                = CreateDefaultSubobject<USkyAtmosphereComponent>("Atmosphere");
	Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform;

	// Settings
	PrimaryActorTick.bCanEverTick = true;
	bRelevantForLevelBounds       = false;
	SetActorEnableCollision(false);
}

/*----------------------------------------------------
    Implementation
----------------------------------------------------*/

void ANovaPlanetarium::BeginPlay()
{
	Super::BeginPlay();
	NLOG("ANovaPlanetarium::BeginPlay");

	// Check everything
	NCHECK(SunRotator);
	NCHECK(Sunlight);
	NCHECK(Skylight);
	NCHECK(Skybox);
	NCHECK(Atmosphere);

	// Get game state
	const UNeutronAssetManager* AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);

	// Find all owned mesh components & all celestial bodies
	TArray<UActorComponent*> MeshComponents;
	GetComponents(UStaticMeshComponent::StaticClass(), MeshComponents);
	TArray<const UNovaCelestialBody*> CelestialBodies = AssetManager->GetAssets<UNovaCelestialBody>();

	// For each known celestial body, map it to a component
	PlanetBody = AssetManager->GetDefaultAsset<UNovaCelestialBody>();
	for (const UNovaCelestialBody* Body : CelestialBodies)
	{
		if (Body->Body == nullptr)
		{
			SunBody = Body;
		}

		for (UActorComponent* Component : MeshComponents)
		{
			if (Component->GetName() == Body->PlanetariumName.ToString())
			{
				NLOG("ANovaPlanetarium::BeginPlay : found mesh for '%s'", *Body->PlanetariumName.ToString());

				UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
				Mesh->CreateAndSetMaterialInstanceDynamic(0);
				CelestialToComponent.Add(Body, Mesh);
			}
		}
	}

	NCHECK(CelestialToComponent.Num() == CelestialBodies.Num());

	Sunlight->SetAtmosphereSunLight(true);
}

void ANovaPlanetarium::Tick(float DeltaTime)
{
	// Check everything
	NCHECK(SunRotator);
	NCHECK(Sunlight);
	NCHECK(Skylight);
	NCHECK(Skybox);
	NCHECK(Atmosphere);

	// Get game state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	const UNeutronAssetManager* AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();

	if (PlayerLocation)
	{
		// Process a planet or moon
		auto ProcessGenericPlanetaryBody = [this, GameState, PlayerLocation](
											   const UNovaCelestialBody* Body, UStaticMeshComponent* Component, bool IsMainPlanet)
		{
			// Spin angle
			const double RelativeBodySpinTime =
				FMath::Fmod(GameState->GetCurrentTime().AsMinutes(), static_cast<double>(Body->RotationPeriod));
			const double RelativeBodySpinAngle = 360.0 * (RelativeBodySpinTime / Body->RotationPeriod);
			const double AbsoluteBodySpinAngle = Body->Phase + RelativeBodySpinAngle;

			double  CurrentAngle    = -AbsoluteBodySpinAngle;
			FVector CurrentPosition = FVector::ZeroVector;

			// Planet
			if (IsMainPlanet)
			{
				// Get position
				const FVector2D PlayerCartesianLocation = PlayerLocation->GetCartesianLocation();
				const double    OrbitRotationAngle =
					FMath::RadiansToDegrees(FVector(PlayerCartesianLocation.X, PlayerCartesianLocation.Y, 0).HeadingAngle());
				CurrentAngle -= -OrbitRotationAngle;
				CurrentPosition = -FVector(0, 0, PlayerCartesianLocation.Size()) * 1000 * 100;

				// Apply sun & atmosphere
				SunRotator->SetWorldLocation(CurrentPosition);
				Atmosphere->SetWorldLocation(CurrentPosition);
				Atmosphere->BottomRadius = Body->Radius;
				CurrentSunSkyAngle       = -OrbitRotationAngle;

				// Feed light direction
				const double LightOffsetRatio = (CurrentSunSkyAngle - CurrentPosition.Rotation().Yaw) / 360.0f;
				Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0))->SetScalarParameterValue("LightOffsetRatio", LightOffsetRatio);
			}

			// Moon
			else
			{
				const FNovaOrbitGeometry   OrbitGeometry     = FNovaOrbitGeometry(Body, Body->Altitude.GetValue(), 0);
				const FNovaOrbit           Orbit             = FNovaOrbit(OrbitGeometry, FNovaTime());
				const double               CurrentPhase      = Orbit.Geometry.GetPhase<true>(GameState->GetCurrentTime());
				const FNovaOrbitalLocation OrbitalLocation   = FNovaOrbitalLocation(OrbitGeometry, -CurrentPhase);
				const FVector2D            CartesianLocation = OrbitalLocation.GetCartesianLocation();

				CurrentPosition = FVector(0, CartesianLocation.X, CartesianLocation.Y) * 1000 * 100;
			}

			// Apply transforms
			Component->SetWorldLocation(CurrentPosition);
			Component->SetWorldRotation(FRotator(CurrentAngle, 90, 90));
			Component->SetWorldScale3D(2.0 * Body->Radius * 1000 * FVector(1, 1, 1));
		};

		// Main planet
		ProcessGenericPlanetaryBody(PlanetBody, CelestialToComponent[PlanetBody], true);

		// Sun
		if (IsValid(CelestialToComponent[SunBody]))
		{
			const double SunDistanceFromPlanet = PlanetBody->Altitude.GetValue() * 1000 * 100;

			CelestialToComponent[SunBody]->SetRelativeLocation(FVector(-SunDistanceFromPlanet, 0, 0));
			CelestialToComponent[SunBody]->SetWorldScale3D(2.0 * SunBody->Radius * 1000 * FVector(1, 1, 1));

			SunRotator->SetWorldRotation(FRotator(CurrentSunSkyAngle, 90, 0));
			Skybox->SetWorldRotation(FRotator(CurrentSunSkyAngle, 90, 90));
		}

		// Moons
		for (const TPair<const UNovaCelestialBody*, UStaticMeshComponent*> BodyAndComponent : CelestialToComponent)
		{
			const UNovaCelestialBody* Body      = BodyAndComponent.Key;
			UStaticMeshComponent*     Component = BodyAndComponent.Value;

			if (IsValid(Component) && Body != SunBody && Body != PlanetBody)
			{
				ProcessGenericPlanetaryBody(Body, Component, false);
			}
		}

		// Rotate billboards around the sky, with the assumption that they're player facing
		TArray<UStaticMeshComponent*> BillboardCandidates;
		GetComponents<UStaticMeshComponent>(BillboardCandidates);
		for (UStaticMeshComponent* BillboardCandidate : BillboardCandidates)
		{
			if (BillboardCandidate->GetName().Contains("Billboard"))
			{
				BillboardCandidate->SetWorldRotation(FRotator(90 - CurrentSunSkyAngle, -90, 0));
			}
		}
	}
};

FVector ANovaPlanetarium::GetSunDirection() const
{
	return Sunlight->GetDirection();
}

FVector ANovaPlanetarium::GetSunLocation() const
{
	return IsValid(CelestialToComponent[SunBody]) ? CelestialToComponent[SunBody]->GetComponentLocation() : FVector::ZeroVector;
}

FVector ANovaPlanetarium::GetPlanetLocation() const
{
	return Atmosphere->GetComponentLocation();
}
