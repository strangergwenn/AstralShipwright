// Nova project - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaArea.generated.h"

/*----------------------------------------------------
    Description types
----------------------------------------------------*/

/** Planet description */
UCLASS(ClassGroup = (Nova))
class UNovaPlanet : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Brush
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	FSlateBrush Image;
};

/** World area description */
UCLASS(ClassGroup = (Nova))
class UNovaArea : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Planet associated with the level
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	const UNovaPlanet* Planet;

	// Sub-level to load
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	FName LevelName;

	// Altitude in kilometers
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Altitude;

	// Initial phase on the orbit in degrees
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float Phase;
};
