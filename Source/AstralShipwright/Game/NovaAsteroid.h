// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NovaAsteroidSimulationComponent.h"

#include "NovaAsteroid.generated.h"

/** Physical asteroid representation */
UCLASS(ClassGroup = (Nova))
class ANovaAsteroid : public AActor
{
	GENERATED_BODY()

public:
	ANovaAsteroid();

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	virtual void Tick(float DeltaTime) override;

	/** Setup the asteroid effects */
	void Initialize(const FNovaAsteroid& InAsteroid);

	/** Check for assets loading */
	bool IsLoadingAssets() const
	{
		return LoadingAssets;
	}

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

	/** Finish spawning */
	void PostLoadInitialize();

	/** Run the movement process */
	void ProcessMovement();

	/** Run the dust particles */
	void ProcessDust();

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Main asteroid mesh
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent* AsteroidMesh;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Material
	UPROPERTY()
	class UMaterialInstanceDynamic* MaterialInstance;

	// General state
	bool          LoadingAssets;
	FNovaAsteroid Asteroid;

	// Dust state
	TArray<FVector>                  DustSources;
	TArray<class UNiagaraComponent*> DustEmitters;
};
