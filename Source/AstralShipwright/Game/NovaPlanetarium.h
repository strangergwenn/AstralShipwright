﻿// Astral Shipwright - Gwennaël Arbona

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

	virtual void Tick(float DeltaTime) override;

	/** Get the physical sun direction */
	FVector GetSunDirection() const;

	/** Get the sun location */
	FVector GetSunLocation() const;

	/** Get the dominant body location */
	FVector GetPlanetLocation() const;

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:

	// Sun parent component
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USceneComponent* SunRotator;

	// Sun light for atmosphere lighting
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UDirectionalLightComponent* Sunlight;

	// Sky light
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USkyLightComponent* Skylight;

	// Sky mesh
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* Skybox;

	// Atmosphere
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class USkyAtmosphereComponent* Atmosphere;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Sun body
	UPROPERTY()
	const class UNovaCelestialBody* SunBody;

	// Planet body
	UPROPERTY()
	const class UNovaCelestialBody* PlanetBody;

	// Celestial body map
	UPROPERTY()
	TMap<const class UNovaCelestialBody*, class UStaticMeshComponent*> CelestialToComponent;

	// General state
	double CurrentSunSkyAngle;
};
