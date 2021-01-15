// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Game/NovaGameTypes.h"
#include "NovaSpacecraftTypes.generated.h"


/*----------------------------------------------------
	General spacecraft types
----------------------------------------------------*/

/** Equipment requirement types */
UENUM()
enum class ENovaEquipmentType : uint8
{
	Standard,
	Engine
};

/** Type of bulkhead to use */
UENUM()
enum class ENovaAssemblyBulkheadType : uint8
{
	None,
	Standard,
	Skirt,
	Outer,
};

/** Type of skirt piping to use */
UENUM()
enum class ENovaAssemblySkirtPipingType : uint8
{
	None,
	Simple,
	Connection
};

/** Possible hull styles */
UENUM()
enum class ENovaAssemblyHullType : uint8
{
	None,
	PlasticFabric,
	MetalFabric
};

/** Possible construction element types */
enum class ENovaAssemblyElementType : uint8
{
	Module,
	Structure,
	Equipment,
	Wiring,
	Hull
};

/** Single construction element */
struct FNovaAssemblyElement
{
	FNovaAssemblyElement()
	{}

	FNovaAssemblyElement(ENovaAssemblyElementType T)
		: Type(T)
	{}

	FSoftObjectPath           Asset;
	ENovaAssemblyElementType  Type;
	class INovaMeshInterface* Mesh = nullptr;
};

/** Compartment processing delegate */
DECLARE_DELEGATE_ThreeParams(FNovaAssemblyCallback, FNovaAssemblyElement&, TSoftObjectPtr<UObject>, TSubclassOf<UPrimitiveComponent>);
