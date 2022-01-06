// Astral Shipwright - Gwennaël Arbona

#include "NovaAsteroid.h"

#include "NovaGameState.h"
#include "NovaOrbitalSimulationComponent.h"

#include "System/NovaAssetManager.h"

#include "Components/StaticMeshComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaAsteroid::ANovaAsteroid() : Super(), LoadingAssets(false)
{
	// Create the main mesh
	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Asteroid"));
	SetRootComponent(AsteroidMesh);

	// Defaults
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(false);
	bAlwaysRelevant = true;
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void ANovaAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsLoadingAssets())
	{
		ProcessMovement();
	}
}

void ANovaAsteroid::Initialize(const FNovaAsteroid& InAsteroid)
{
	// Initialize to safe defaults
	LoadingAssets = true;
	Asteroid      = InAsteroid;
	SetActorLocation(FVector(0, 0, -1000 * 1000 * 100));

	// Load assets and resume initializing later
	UNovaAssetManager::Get()->LoadAssets({Asteroid.Mesh.ToSoftObjectPath(), Asteroid.DustEffect.ToSoftObjectPath()},
		FStreamableDelegate::CreateUObject(this, &ANovaAsteroid::PostLoadInitialize));
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaAsteroid::PostLoadInitialize()
{
	NLOG("ANovaAsteroid::PostLoadInitialize : ready to show '%s'", *Asteroid.Identifier.ToString(EGuidFormats::Short));
	AsteroidMesh->SetStaticMesh(Asteroid.Mesh.Get());
	LoadingAssets = false;
}

void ANovaAsteroid::ProcessMovement()
{
	// Get basic game state pointers
	const ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	const UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Get locations
	const FVector2D PlayerLocation       = OrbitalSimulation->GetPlayerCartesianLocation();
	const FVector2D AsteroidLocation     = OrbitalSimulation->GetAsteroidLocation(Asteroid.Identifier).GetCartesianLocation();
	FVector2D       LocationInKilometers = AsteroidLocation - PlayerLocation;

	// Transform the location accounting for angle and scale
	const FVector2D PlayerDirection       = PlayerLocation.GetSafeNormal();
	double          PlayerAngle           = 180 + FMath::RadiansToDegrees(FMath::Atan2(PlayerDirection.X, PlayerDirection.Y));
	LocationInKilometers                  = LocationInKilometers.GetRotated(PlayerAngle);
	const FVector RelativeOrbitalLocation = FVector(0, -LocationInKilometers.X, LocationInKilometers.Y) * 1000 * 100;

	SetActorLocation(RelativeOrbitalLocation);
}
