// Astral Shipwright - Gwennaël Arbona

#include "NovaAsteroid.h"
#include "NovaAsteroidSimulationComponent.h"

#include "Components/StaticMeshComponent.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaAsteroid::ANovaAsteroid() : Super()
{
	// Create the main mesh
	AsteroidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Asteroid"));
	SetRootComponent(AsteroidMesh);

	// Defaults
	SetActorTickEnabled(true);
	SetReplicates(false);
	bAlwaysRelevant = true;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void ANovaAsteroid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANovaAsteroid::Initialize(const FNovaAsteroid& Asteroid)
{}
