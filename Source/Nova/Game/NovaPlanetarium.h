// Nova project - Gwennaël Arbona

#pragma once

#include "GameFramework/Actor.h"

#include "NovaPlanetarium.generated.h"

/** Sky simulation actor */
UCLASS()
class ANovaPlanetarium : public AActor
{
	GENERATED_BODY()

public:
	ANovaPlanetarium();

	/*----------------------------------------------------
	    Planetarium implementation
	----------------------------------------------------*/

	void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Sun body
	UPROPERTY()
	const class UNovaCelestialBody* RootBody;

	// Celestial body map
	UPROPERTY()
	TMap<const class UNovaCelestialBody*, class UStaticMeshComponent*> CelestialToComponent;
};
