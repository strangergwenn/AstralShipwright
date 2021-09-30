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
	    Components
	----------------------------------------------------*/

protected:
	// Sun body
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	const class UNovaCelestialBody* SunBody;

	// Sun light for atmosphere lighting
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UDirectionalLightComponent* Sunlight;

	// Sky light
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USkyLightComponent* Skylight;

	// Sky mesh
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* Skybox;

	// Sky light
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USkyAtmosphereComponent* Atmosphere;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Celestial body map
	UPROPERTY()
	TMap<const class UNovaCelestialBody*, class UStaticMeshComponent*> CelestialToComponent;
};
