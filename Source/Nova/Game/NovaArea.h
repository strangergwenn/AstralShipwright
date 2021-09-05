// Nova project - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaOrbitalSimulatioNTypes.h"
#include "NovaArea.generated.h"

/*----------------------------------------------------
    Description types
----------------------------------------------------*/

/** Resource trade metadata */
USTRUCT()
struct FNovaResourceTrade
{
	GENERATED_BODY()

	FNovaResourceTrade() : Resource(nullptr), PriceModifier(ENovaPriceModifier::Average), ForSale(false)
	{}

public:
	// Resource this data applies to
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UNovaResource* Resource;

	// Resource price modifier
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaPriceModifier PriceModifier;

	// Resource is being sold here as opposed to bought
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool ForSale;
};

/** World area description */
UCLASS(ClassGroup = (Nova))
class UNovaArea : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Body orbited
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UNovaCelestialBody* Body;

	// Altitude in kilometers
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Altitude;

	// Initial phase on the orbit in degrees
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Phase;

	// Sub-level to load
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FName LevelName;

	// Area description
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FText Description;

	// Resources sold in this area
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaResourceTrade> ResourceTradeMetadata;
};
