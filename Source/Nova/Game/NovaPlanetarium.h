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
	const class UNovaCelestialBody* SunBody;

	// Sky mesh
	UPROPERTY()
	class UStaticMeshComponent* SkyboxComponent;

	// Sun light
	UPROPERTY()
	class UDirectionalLightComponent* SunlightComponent;

	// Sky light
	UPROPERTY()
	class USkyLightComponent* SkylightComponent;

	// Celestial body map
	UPROPERTY()
	TMap<const class UNovaCelestialBody*, class UStaticMeshComponent*> CelestialToComponent;

	// Atmosphere meshes
	UPROPERTY()
	TArray<class UStaticMeshComponent*> AtmosphereComponents;
};
