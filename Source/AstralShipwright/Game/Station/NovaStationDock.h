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
	// Paint color to apply on all meshes
	UPROPERTY(Category = Nova, EditAnywhere)
	FLinearColor PaintColor;

	// Color to apply on spotlights and signs
	UPROPERTY(Category = Nova, EditAnywhere)
	FLinearColor LightColor;

	// Color to apply on decals
	UPROPERTY(Category = Nova, EditAnywhere)
	FLinearColor DecalColor;

	// Ring index
	UPROPERTY(Category = Nova, EditAnywhere)
	float DirtyIntensity;
};
