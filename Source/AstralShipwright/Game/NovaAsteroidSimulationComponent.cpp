// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaAsteroidSimulationComponent.h"

#include "NovaAsteroid.h"
#include "NovaOrbitalSimulationComponent.h"
#include "NovaGameState.h"

#include "Curves/CurveFloat.h"

// Definitions
static constexpr int32 AltitudeDistributionValues = 100;
static constexpr int32 IntegralValues             = 100;
static constexpr int32 AsteroidSpawnDistanceKm    = 500;
static constexpr int32 AsteroidDespawnDistanceKm  = 600;

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
			double               DistanceFromPlayer = IdentifierAndLocation.Value.GetDistanceTo(*PlayerLocation);
			const FNovaAsteroid& Asteroid           = AsteroidDatabase[IdentifierAndLocation.Key];

			// Spawn
			if (DistanceFromPlayer < AsteroidSpawnDistanceKm && PhysicalAsteroidDatabase.Find(IdentifierAndLocation.Key) == nullptr)
			{
				ANovaAsteroid* NewAsteroid = GetWorld()->SpawnActor<ANovaAsteroid>();
				NCHECK(NewAsteroid);
				NewAsteroid->Initialize(Asteroid);

				NLOG("UNovaAsteroidSimulationComponent::TickComponent : spawning '%s'",
					*IdentifierAndLocation.Key.ToString(EGuidFormats::Short));

				PhysicalAsteroidDatabase.Add(IdentifierAndLocation.Key, NewAsteroid);
			}

			// De-spawn
			else if (DistanceFromPlayer > AsteroidDespawnDistanceKm)
			{
				ANovaAsteroid** AsteroidEntry = PhysicalAsteroidDatabase.Find(IdentifierAndLocation.Key);
				if (AsteroidEntry && !(*AsteroidEntry)->IsLoadingAssets())
				{
					NLOG("UNovaAsteroidSimulationComponent::TickComponent : removing '%s'",
						*IdentifierAndLocation.Key.ToString(EGuidFormats::Short));

					(*AsteroidEntry)->Destroy();
					PhysicalAsteroidDatabase.Remove(IdentifierAndLocation.Key);
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
		Asteroid            = FNovaAsteroid(RandomStream, AsteroidConfiguration->Body, Altitude, Phase);
		Asteroid.Mesh       = AsteroidConfiguration->Meshes[RandomStream.RandHelper(AsteroidConfiguration->Meshes.Num())];
		Asteroid.DustEffect = AsteroidConfiguration->DustEffects[RandomStream.RandHelper(AsteroidConfiguration->DustEffects.Num())];

		SpawnedAsteroids++;
		return true;
	}

	return false;
}
