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
};
