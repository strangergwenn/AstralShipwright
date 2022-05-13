// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaAsteroidSimulationComponent.generated.h"

/** Asteroid catalog & metadata */
UCLASS(ClassGroup = (Nova))
class UNovaAsteroidConfiguration : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Body orbited
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UNovaCelestialBody* Body;

	// Total amount of asteroids to spawn in the entire game
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 TotalAsteroidCount;

	// Orbital altitude distribution
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UCurveFloat* AltitudeDistribution;

	// Meshes to use on asteroids
	UPROPERTY(Category = Assets, EditDefaultsOnly)
	TArray<TSoftObjectPtr<class UStaticMesh>> Meshes;

	// Dust particle effect to use on asteroids
	UPROPERTY(Category = Assets, EditDefaultsOnly)
	TSoftObjectPtr<class UNiagaraSystem> DustEffect;
};

/** Asteroid data object */
struct FNovaAsteroid
{
	FNovaAsteroid()
		: Identifier(FGuid::NewGuid())
		, Body(nullptr)
		, Altitude(0)
		, Phase(0)

		, Mesh()
		, Scale(0)
		, Rotation()
		, EffectsCount(0)
		, MineralsSeed(0)
	{}

	FNovaAsteroid(FRandomStream& RandomStream, const UNovaAsteroidConfiguration* AsteroidConfiguration, double A, double P);

	// Identifier
	FGuid Identifier;

	// Orbital parameters
	const class UNovaCelestialBody* Body;
	double                          Altitude;
	double                          Phase;

	// Physical characteristics
	TSoftObjectPtr<class UStaticMesh>    Mesh;
	TSoftObjectPtr<class UNiagaraSystem> DustEffect;
	float                                Scale;
	FRotator                             Rotation;
	int32                                EffectsCount;
	int32                                MineralsSeed;
};

/** Asteroid spawning & update manager */
UCLASS(ClassGroup = (Nova))
class UNovaAsteroidSimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaAsteroidSimulationComponent();

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Reset the component */
	void Initialize(const UNovaAsteroidConfiguration* Configuration);

	/** Get a specific asteroid */
	const FNovaAsteroid* GetAsteroid(FGuid Identifier) const
	{
		return AsteroidDatabase.Find(Identifier);
	}

	/** Get all asteroids */
	const TMap<FGuid, FNovaAsteroid>& GetAsteroids() const
	{
		return AsteroidDatabase;
	}

	/** Get a physical asteroid */
	const class ANovaAsteroid* GetPhysicalAsteroid(FGuid Identifier) const
	{
		ANovaAsteroid* const* AsteroidPtr = PhysicalAsteroidDatabase.Find(Identifier);
		return AsteroidPtr ? *AsteroidPtr : nullptr;
	}

	/** Load the physical asteroid for given identifier */
	void SetRequestedPhysicalAsteroid(FGuid Identifier)
	{
		AlwaysLoadedAsteroid = Identifier;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Check if a new asteroid can be created and get its details */
	bool CreateAsteroid(double& Altitude, double& Phase, FNovaAsteroid& Asteroid);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Asteroid setup
	const UNovaAsteroidConfiguration* AsteroidConfiguration;

	// Asteroid spawning
	int32                    SpawnedAsteroids;
	FRandomStream            RandomStream;
	TMultiMap<int32, double> IntegralToAltitudeTable;

	// Asteroid databases
	FGuid                             AlwaysLoadedAsteroid;
	TMap<FGuid, FNovaAsteroid>        AsteroidDatabase;
	TMap<FGuid, class ANovaAsteroid*> PhysicalAsteroidDatabase;
};
