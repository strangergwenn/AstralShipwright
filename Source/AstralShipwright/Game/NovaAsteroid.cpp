// Astral Shipwright - Gwennaël Arbona

#include "NovaAsteroid.h"

#include "NovaPlanetarium.h"
#include "NovaGameState.h"
#include "NovaOrbitalSimulationComponent.h"

#include "System/NovaAssetManager.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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

void ANovaAsteroid::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	if (!IsValid(GetOwner()))
	{
		NLOG("ANovaAsteroid::BeginPlay : spawning as fallback");

		const UNovaAsteroidConfiguration* AsteroidConfiguration = UNovaAssetManager::Get()->GetDefaultAsset<UNovaAsteroidConfiguration>();
		NCHECK(AsteroidConfiguration);
		FRandomStream Random;

		Initialize(FNovaAsteroid(Random, AsteroidConfiguration, 0, 0));
	}
#endif    // WITH_EDITOR
}

void ANovaAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsLoadingAssets())
	{
		ProcessMovement();
		ProcessDust();
	}
}

void ANovaAsteroid::Initialize(const FNovaAsteroid& InAsteroid)
{
	// Initialize to safe defaults
	LoadingAssets = true;
	Asteroid      = InAsteroid;
	SetActorRotation(InAsteroid.Rotation);
	SetActorScale3D(Asteroid.Scale * FVector(1, 1, 1));

	// Default game path : load assets and resume initializing later
	if (IsValid(GetOwner()))
	{
		SetActorLocation(FVector(0, 0, -1000 * 1000 * 100));

		UNovaAssetManager::Get()->LoadAssets({Asteroid.Mesh.ToSoftObjectPath(), Asteroid.DustEffect.ToSoftObjectPath()},
			FStreamableDelegate::CreateUObject(this, &ANovaAsteroid::PostLoadInitialize));
	}
#if WITH_EDITOR
	else
	{
		Asteroid.Mesh.LoadSynchronous();
		Asteroid.DustEffect.LoadSynchronous();
		PostLoadInitialize();
	}
#endif    // WITH_EDITOR
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaAsteroid::PostLoadInitialize()
{
	LoadingAssets = false;

	NLOG("ANovaAsteroid::PostLoadInitialize : ready to show '%s'", *Asteroid.Identifier.ToString(EGuidFormats::Short));

	// Setup asteroid mesh & material
	NCHECK(Asteroid.Mesh.IsValid());
	AsteroidMesh->SetStaticMesh(Asteroid.Mesh.Get());
	MaterialInstance = AsteroidMesh->CreateAndSetMaterialInstanceDynamic(0);
	MaterialInstance->SetScalarParameterValue("AsteroidScale", Asteroid.Scale);

	// Create random effects
	for (int32 Index = 0; Index < Asteroid.EffectsCount; Index++)
	{
		DustSources.Add(FMath::VRand());

		UNiagaraComponent* Emitter = UNiagaraFunctionLibrary::SpawnSystemAttached(Asteroid.DustEffect.Get(), AsteroidMesh, NAME_None,
			GetActorLocation(), FRotator::ZeroRotator, EAttachLocation::KeepWorldPosition, false);

		Emitter->SetWorldScale3D(Asteroid.Scale * FVector(1, 1, 1));

		DustEmitters.Add(Emitter);
	}
}

void ANovaAsteroid::ProcessMovement()
{
	if (IsValid(GetOwner()))
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
}

void ANovaAsteroid::ProcessDust()
{
	// Get the planetarium
	TArray<AActor*> PlanetariumCandidates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlanetarium::StaticClass(), PlanetariumCandidates);
	NCHECK(PlanetariumCandidates.Num() > 0);
	const ANovaPlanetarium* Planetarium = Cast<ANovaPlanetarium>(PlanetariumCandidates[0]);

	// Get world data
	FVector AsteroidLocation = GetActorLocation();
	FVector SunDirection     = Planetarium->GetSunDirection();
	float   CollisionSize    = AsteroidMesh->GetCollisionShape().GetExtent().Size();

	// Compute new FX locations
	for (int32 Index = 0; Index < DustEmitters.Num(); Index++)
	{
		FVector RandomDirection = FVector::CrossProduct(SunDirection, DustSources[Index]);
		RandomDirection.Normalize();
		FVector StartPoint = AsteroidLocation + RandomDirection * CollisionSize;

		// Trace params
		FHitResult            HitResult(ForceInit);
		FCollisionQueryParams TraceParams(FName(TEXT("Asteroid Trace")), false, NULL);
		TraceParams.bTraceComplex           = true;
		TraceParams.bReturnPhysicalMaterial = false;
		ECollisionChannel CollisionChannel  = ECollisionChannel::ECC_WorldDynamic;

		// Trace
		bool FoundHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartPoint, AsteroidLocation, CollisionChannel, TraceParams);
		if (FoundHit && HitResult.GetActor() == this && IsValid(DustEmitters[Index]))
		{
			FVector EffectLocation = HitResult.Location;

			if (!DustEmitters[Index]->IsActive())
			{
				DustEmitters[Index]->Activate();
			}
			DustEmitters[Index]->SetWorldLocation(EffectLocation);
			DustEmitters[Index]->SetWorldRotation(SunDirection.Rotation());
		}
		else
		{
			DustEmitters[Index]->Deactivate();
		}
	}
}
