// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaAsteroid.h"

#include "Curves/CurveFloat.h"

// Definitions
static constexpr int32 AltitudeDistributionValues = 100;
static constexpr int32 IntegralValues             = 100;

/*----------------------------------------------------
    Asteroid spawning
----------------------------------------------------*/

void FNovaAsteroidSpawner::Start(const UNovaAsteroidConfiguration* Configuration)
{
	NCHECK(Configuration != nullptr);
	NCHECK(Configuration->AltitudeDistribution != nullptr);

	// Initialize our state
	AsteroidConfiguration = Configuration;
	SpawnedAsteroids      = 0;
	RandomStream          = FRandomStream();

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
}

bool FNovaAsteroidSpawner::GetNextAsteroid(double& Altitude, double& Phase, FNovaAsteroid& Asteroid)
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
		Asteroid      = FNovaAsteroid(RandomStream, AsteroidConfiguration->Body, Altitude, Phase);
		Asteroid.Mesh = AsteroidConfiguration->Meshes[RandomStream.RandHelper(AsteroidConfiguration->Meshes.Num())];

		SpawnedAsteroids++;
		return true;
	}

	return false;
}
