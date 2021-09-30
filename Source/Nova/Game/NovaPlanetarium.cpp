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
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Materials/MaterialInstanceConstant.h"

#define CELESTIAL_SCALE 1.0

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaPlanetarium::ANovaPlanetarium() : Super()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	// Create sunlight
	Sunlight = CreateDefaultSubobject<UDirectionalLightComponent>("Sunlight");
	Sunlight->SetupAttachment(RootComponent);
	Sunlight->bUsedAsAtmosphereSunLight = true;

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
	NCHECK(Sunlight);
	NCHECK(Skylight);
	NCHECK(Skybox);
	NCHECK(Atmosphere);

	// Get game state
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
			SunBody = Body;
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
	// Check everything
	NCHECK(Sunlight);
	NCHECK(Skylight);
	NCHECK(Skybox);
	NCHECK(Atmosphere);

	// Get game state
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();
	NCHECK(PlayerLocation);

	float CurrentSunAngle = 0;
	float CurrentSkyAngle = 0;

	// Process bodies
	for (const TPair<const UNovaCelestialBody*, UStaticMeshComponent*> BodyAndComponent : CelestialToComponent)
	{
		const UNovaCelestialBody* Body      = BodyAndComponent.Key;
		UStaticMeshComponent*     Component = BodyAndComponent.Value;

		if (IsValid(Component))
		{
			float   CurrentAngle    = 0;
			FVector CurrentPosition = FVector::ZeroVector;

			// Sun
			if (Body == SunBody)
			{
				CurrentPosition = -FVector(Body->Radius + PlayerLocation->Geometry.Body->Altitude.GetValue(), 0, 0);

				CurrentPosition = CurrentPosition.RotateAngleAxis(85, FVector(0, 0, 1));    // DEBUG
				// CurrentPosition = CurrentPosition.RotateAngleAxis(45, FVector(0, 0, 1));    // DEBUG

				CurrentSunAngle = FMath::RadiansToDegrees((-CurrentPosition).HeadingAngle());
			}

			// Planet
			else
			{
				const double CurrentRelativePeriod =
					FMath::Fmod(GameState->GetCurrentTime().AsMinutes(), static_cast<double>(Body->RotationPeriod));
				const double BodyRotationAngle = Body->Phase + 360.0 * (CurrentRelativePeriod / Body->RotationPeriod);

				const FVector2D PlayerCartesianLocation = PlayerLocation->GetCartesianLocation();
				const double    OrbitRotationAngle =
					FMath::RadiansToDegrees(FVector(PlayerCartesianLocation.X, PlayerCartesianLocation.Y, 0).HeadingAngle());

				CurrentAngle    = -BodyRotationAngle - OrbitRotationAngle;
				CurrentPosition = FVector(Body->Radius + PlayerLocation->Geometry.StartAltitude, 0, 0);
				CurrentSkyAngle = CurrentAngle;

				// Atmosphere
				Atmosphere->SetWorldLocation(CELESTIAL_SCALE * CurrentPosition * 1000 * 100);
				Atmosphere->BottomRadius = CELESTIAL_SCALE * Body->Radius;
			}

			// Apply transforms
			Component->SetWorldRotation(FRotator(0, CurrentAngle, 0));
			Component->SetWorldLocation(CELESTIAL_SCALE * CurrentPosition * 1000 * 100);
			Component->SetWorldScale3D(CELESTIAL_SCALE * 2.0 * Body->Radius * 1000 * FVector(1, 1, 1));
		}
	}

	// Rotate sky box & sun
	Sunlight->SetWorldRotation(FRotator(-5, CurrentSunAngle, 0));
	Skybox->SetWorldRotation(FRotator(0, CurrentSkyAngle, 0));
};
