// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

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
	void Initialize(const struct FNovaAsteroid& Asteroid);

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

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
};
