// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NovaStationDock.generated.h"

/** Station dock actor */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class ANovaStationDock : public AActor
{
	GENERATED_BODY()

public:
	ANovaStationDock();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Map of meshes that should be mapped to a Niagara particle system
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	TMap<const class UStaticMesh*, class UNiagaraSystem*> MeshToSystem;

	// Distance to add from center
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float SystemDistance;
};
