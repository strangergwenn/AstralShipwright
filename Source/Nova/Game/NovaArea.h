// Nova project - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaArea.generated.h"

/*----------------------------------------------------
    Description types
----------------------------------------------------*/

/** Large number type for planet mass */
USTRUCT()
struct FNovaPlanetMass
{
	GENERATED_BODY()

	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Value;

	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Exponent;
};

/** Planet description */
UCLASS(ClassGroup = (Nova))
class UNovaPlanet : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	/** Get the mass of the planet in kg */
	double GetMass() const
	{
		return static_cast<double>(Mass.Value) * FMath::Pow(10, static_cast<double>(Mass.Exponent));
	}

	/** Get the gravitational parameter in m3/s² */
	double GetGravitationalParameter() const
	{
		constexpr double GravitationalConstant = 6.674e-11;
		return GravitationalConstant * GetMass();
	}

	/** Get a radius value in meters from altitude above ground in km */
	double GetRadius(float Altitude) const
	{
		return (static_cast<double>(Radius) + static_cast<double>(Altitude)) * 1000.0;
	}

public:
	// Brush
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FSlateBrush Image;

	// Planet radius in km
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Radius;

	// Mass in kg with exponent stored separately
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FNovaPlanetMass Mass;
};

/** Sold resource */
USTRUCT()
struct FNovaResourceSale
{
	GENERATED_BODY()

	FNovaResourceSale() : Resource(nullptr), PriceModifier(ENovaPriceModifier::Average)
	{}

public:
	// Resource being sold here
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const class UNovaResource* Resource;

	// Resource price modifier
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaPriceModifier PriceModifier;
};

/** World area description */
UCLASS(ClassGroup = (Nova))
class UNovaArea : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Planet associated with the level
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	const UNovaPlanet* Planet;

	// Sub-level to load
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FName LevelName;

	// Altitude in kilometers
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Altitude;

	// Initial phase on the orbit in degrees
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Phase;

	// Resources bought in this area
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaResourceSale> ResourcesBought;

	// Resources sold in this area
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaResourceSale> ResourcesSold;
};
