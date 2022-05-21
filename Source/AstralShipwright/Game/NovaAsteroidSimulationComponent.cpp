// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaAsteroidSimulationComponent.h"

#include "NovaAsteroid.h"
#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Nova.h"

#include "Curves/CurveFloat.h"

// Definitions
static constexpr int32 AltitudeDistributionValues = 100;
static constexpr int32 IntegralValues             = 100;
static constexpr int32 AsteroidSpawnDistanceKm    = 500;
static constexpr int32 AsteroidDespawnDistanceKm  = 600;

/*----------------------------------------------------
    Asteroid
----------------------------------------------------*/

FNovaAsteroid::FNovaAsteroid(FRandomStream& RandomStream, const UNovaAsteroidConfiguration* AsteroidConfiguration, double A, double P)
	: Identifier(FGuid(RandomStream.RandHelper(MAX_int32), RandomStream.RandHelper(MAX_int32), RandomStream.RandHelper(MAX_int32),
		  RandomStream.RandHelper(MAX_int32)))
	, Body(AsteroidConfiguration->Body)
	, Altitude(A)
	, Phase(P)
	, Scale(RandomStream.FRandRange(90, 130))
	, Rotation(RandomStream.FRandRange(-90, 90), RandomStream.FRandRange(0, 360), 0)
	, EffectsCount(RandomStream.FRandRange(20, 40))
	, MineralsSeed(RandomStream.GetCurrentSeed())
{
	Mesh       = AsteroidConfiguration->Meshes[RandomStream.RandHelper(AsteroidConfiguration->Meshes.Num())];
	DustEffect = AsteroidConfiguration->DustEffect;
}

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaAsteroidSimulationComponent::UNovaAsteroidSimulationComponent() : Super()
{
	// Settings
	PrimaryComponentTick.bCanEverTick = true;
}

/*----------------------------------------------------
    Asteroid runtime processing
----------------------------------------------------*/

void UNovaAsteroidSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Get game state pointers
	ANovaGameState* GameState = Cast<ANovaGameState>(GetOwner());
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const FNovaOrbitalLocation* PlayerLocation = OrbitalSimulation->GetPlayerLocation();

	// Iterate over all asteroids
	if (PlayerLocation)
	{
		for (const TPair<FGuid, FNovaOrbitalLocation>& IdentifierAndLocation : OrbitalSimulation->GetAllAsteroidsLocations())
		{
			FGuid  Identifier         = IdentifierAndLocation.Key;
			double DistanceFromPlayer = IdentifierAndLocation.Value.GetDistanceTo(*PlayerLocation);

			// Spawn
			if (GetPhysicalAsteroid(Identifier) == nullptr &&
				(Identifier == AlwaysLoadedAsteroid || DistanceFromPlayer < AsteroidSpawnDistanceKm))
			{
				ANovaAsteroid* NewAsteroid = GetWorld()->SpawnActor<ANovaAsteroid>();
				NCHECK(NewAsteroid);
				NewAsteroid->SetOwner(GetOwner());
				NewAsteroid->Initialize(AsteroidDatabase[Identifier]);

				NLOG("UNovaAsteroidSimulationComponent::TickComponent : spawning '%s'", *Identifier.ToString(EGuidFormats::Short));

				PhysicalAsteroidDatabase.Add(Identifier, NewAsteroid);
			}

			// De-spawn
			if (GetPhysicalAsteroid(Identifier) != nullptr && !AlwaysLoadedAsteroid.IsValid() &&
				DistanceFromPlayer > AsteroidDespawnDistanceKm)
			{
				ANovaAsteroid** AsteroidEntry = PhysicalAsteroidDatabase.Find(Identifier);
				if (AsteroidEntry && !(*AsteroidEntry)->IsLoadingAssets())
				{
					NLOG("UNovaAsteroidSimulationComponent::TickComponent : removing '%s'", *Identifier.ToString(EGuidFormats::Short));

					(*AsteroidEntry)->Destroy();
					PhysicalAsteroidDatabase.Remove(Identifier);
				}
			}
		}
	}
}

/*----------------------------------------------------
    Asteroid spawning
----------------------------------------------------*/

void UNovaAsteroidSimulationComponent::Initialize(const UNovaAsteroidConfiguration* Configuration)
{
	NLOG("UNovaAsteroidSimulationComponent::Initialize");
	NCHECK(Configuration != nullptr);
	NCHECK(Configuration->AltitudeDistribution != nullptr);

	// Initialize our state
	AsteroidConfiguration = Configuration;
	SpawnedAsteroids      = 0;
	RandomStream          = FRandomStream();
	AsteroidDatabase.Empty();

	// Identify quantization step
	double MinAltitude, MaxAltitude;
	AsteroidConfiguration->AltitudeDistribution->GetTimeRange(MinAltitude, MaxAltitude);
	double AltitudeStep = (MaxAltitude - MinAltitude) / AltitudeDistributionValues;

	// Compute the total integral value for normalization
	double TotalIntegral = 0.0;
	for (double Altitude = MinAltitude; Altitude <= MaxAltitude; Altitude += (MaxAltitude - MinAltitude) / AltitudeDistributionValues)
	{
		TotalIntegral += Configuration->AltitudeDistribution->GetFloatValue(Altitude);
	}

	// Fill the lookup table, mapping integral to the source altitude with quantization for fast runs
	IntegralToAltitudeTable.Empty();
	double CurrentIntegral = 0.0;
	for (double Value = 0.0; Value <= 1.0; Value += 1.0 / AltitudeDistributionValues)
	{
		double Altitude = MinAltitude + Value * (MaxAltitude - MinAltitude);
		CurrentIntegral += Configuration->AltitudeDistribution->GetFloatValue(Altitude);

		int32 RandomKey = FMath::RoundToInt(IntegralValues * CurrentIntegral / TotalIntegral);
		IntegralToAltitudeTable.Add(RandomKey, Altitude);
	}

	// Spawn asteroids
	FNovaAsteroid NewAsteroid;
	double        Altitude, Phase;
	while (CreateAsteroid(Altitude, Phase, NewAsteroid))
	{
		AsteroidDatabase.Add(NewAsteroid.Identifier, NewAsteroid);
	}
}

bool UNovaAsteroidSimulationComponent::CreateAsteroid(double& Altitude, double& Phase, FNovaAsteroid& Asteroid)
{
	NCHECK(AsteroidConfiguration);

	if (SpawnedAsteroids < AsteroidConfiguration->TotalAsteroidCount)
	{
		// Find altitude inputs that match the probability picked
		int32          RandomKey = RandomStream.RandHelper(IntegralValues);
		TArray<double> Altitudes;
		IntegralToAltitudeTable.MultiFind(RandomKey, Altitudes);

		// Found one matching altitude
		int32 Direction = RandomKey < IntegralValues / 2 ? 1 : -1;
		while (IntegralToAltitudeTable.Find(RandomKey) == nullptr)
		{
			RandomKey += Direction;
			NCHECK(RandomKey >= 0 && RandomKey < IntegralValues);
		}

		// Get the altitude & phase
		IntegralToAltitudeTable.MultiFind(RandomKey, Altitudes);
		Altitude = Altitudes[RandomStream.RandHelper(Altitudes.Num() - 1)];
		Phase    = RandomStream.FRandRange(0.0, 360.0);

		// Generate the asteroid itself
		Asteroid = FNovaAsteroid(RandomStream, AsteroidConfiguration, Altitude, Phase);

		SpawnedAsteroids++;
		return true;
	}

	return false;
}
