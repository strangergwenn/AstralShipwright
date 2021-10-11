// Nova project - Gwennaël Arbona

#include "NovaPlanetarium.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationTypes.h"
#include "NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"

#include "Nova/Nova.h"

#include "Materials/MaterialInstanceConstant.h"
#include "EngineUtils.h"
#include "Engine.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaPlanetarium::ANovaPlanetarium() : Super()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

	// Create sunlight
	SunRotator = CreateDefaultSubobject<USceneComponent>("SunRotator");
	SunRotator->SetupAttachment(RootComponent);

	// Create sunlight
	Sunlight = CreateDefaultSubobject<UDirectionalLightComponent>("Sunlight");
	Sunlight->SetupAttachment(SunRotator);
	Sunlight->bUsedAsAtmosphereSunLight        = true;
	Sunlight->bPerPixelAtmosphereTransmittance = true;

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

				UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
				Mesh->CreateAndSetMaterialInstanceDynamic(0);
				CelestialToComponent.Add(Body, Mesh);
			}
		}
	}

	NCHECK(CelestialToComponent.Num() == CelestialBodies.Num());
}

void ANovaPlanetarium::Tick(float DeltaSeconds)
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
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();
	NCHECK(PlayerLocation);

	float CurrentSunSkyAngle    = 0;
	float SunDistanceFromPlanet = 0;

	// Process bodies
	for (const TPair<const UNovaCelestialBody*, UStaticMeshComponent*> BodyAndComponent : CelestialToComponent)
	{
		const UNovaCelestialBody* Body      = BodyAndComponent.Key;
		UStaticMeshComponent*     Component = BodyAndComponent.Value;

		if (IsValid(Component))
		{
			float   CurrentAngle    = 0;
			FVector CurrentPosition = FVector::ZeroVector;

			// Planet
			if (Body != SunBody)
			{
				const double RelativeBodySpinTime =
					FMath::Fmod(GameState->GetCurrentTime().AsMinutes(), static_cast<double>(Body->RotationPeriod));
				const double RelativeBodySpinAngle = 360.0 * (RelativeBodySpinTime / Body->RotationPeriod);
				const double AbsoluteBodySpinAngle = Body->Phase + RelativeBodySpinAngle;

				const FVector2D PlayerCartesianLocation = PlayerLocation->GetCartesianLocation();
				const double    OrbitRotationAngle =
					FMath::RadiansToDegrees(FVector(PlayerCartesianLocation.X, PlayerCartesianLocation.Y, 0).HeadingAngle());

				// Position
				CurrentAngle    = -AbsoluteBodySpinAngle - OrbitRotationAngle;
				CurrentPosition = FVector(Body->Radius + PlayerLocation->Geometry.StartAltitude, 0, 0) * 1000 * 100;

				// This is the only planetary body for now so we'll apply sun & atmosphere there
				{
					SunRotator->SetWorldLocation(CurrentPosition);
					Atmosphere->SetWorldLocation(CurrentPosition);
					Atmosphere->BottomRadius = Body->Radius;

					SunDistanceFromPlanet = Body->Altitude.GetValue() * 1000 * 100;
					CurrentSunSkyAngle    = -OrbitRotationAngle;
				}

				// Add light direction
				float LightOffsetRatio = (CurrentSunSkyAngle - CurrentPosition.Rotation().Yaw) / 360.0f;
				Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0))->SetScalarParameterValue("LightOffsetRatio", LightOffsetRatio);
			}

			// Apply transforms
			Component->SetWorldRotation(FRotator(0, CurrentAngle, 0));
			Component->SetWorldLocation(CurrentPosition);
			Component->SetWorldScale3D(2.0 * Body->Radius * 1000 * FVector(1, 1, 1));
		}
	}

	// Rotate sky box & sun
	CelestialToComponent[SunBody]->SetRelativeLocation(FVector(-SunDistanceFromPlanet, 0, 0));
	SunRotator->SetWorldRotation(FRotator(0, CurrentSunSkyAngle, 0));
	Skybox->SetWorldRotation(FRotator(0, CurrentSunSkyAngle, 0));

	FVector SunDistanceMKM = CelestialToComponent[SunBody]->GetComponentLocation() / (1000 * 1000 * 1000 * 100);

	// Rotate billboards
	TArray<UStaticMeshComponent*> BillboardCandidates;
	GetComponents<UStaticMeshComponent>(BillboardCandidates);
	for (UStaticMeshComponent* BillboardCandidate : BillboardCandidates)
	{
		if (BillboardCandidate->GetName().Contains("Billboard"))
		{
			FRotator Rotation = BillboardCandidate->GetRelativeRotation();
			Rotation.Yaw      = FMath::RadiansToDegrees(BillboardCandidate->GetComponentLocation().HeadingAngle()) + 90;
			BillboardCandidate->SetRelativeRotation(Rotation);
		}
	}
};
