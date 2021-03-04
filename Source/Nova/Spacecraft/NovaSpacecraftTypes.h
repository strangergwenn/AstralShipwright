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
enum class ENovaBulkheadType : uint8
{
	None,
	Standard,
	Skirt,
	Outer,
};

/** Type of skirt piping to use */
UENUM()
enum class ENovaSkirtPipingType : uint8
{
	None,
	Simple,
	Connection
};

/** Possible hull styles */
UENUM()
enum class ENovaHullType : uint8
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

	FNovaAssemblyElement(ENovaAssemblyElementType T) : Type(T)
	{}

	FSoftObjectPath           Asset;
	ENovaAssemblyElementType  Type;
	class INovaMeshInterface* Mesh = nullptr;
};

/** Compartment processing delegate */
DECLARE_DELEGATE_ThreeParams(FNovaAssemblyCallback, FNovaAssemblyElement&, TSoftObjectPtr<UObject>, FNovaAdditionalComponent);

/*----------------------------------------------------
    Additional component mechanism
----------------------------------------------------*/

/** Additional component to spawn alongside a particular object */
USTRUCT()
struct FNovaAdditionalComponent
{
	GENERATED_BODY();

	// Component class to use
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USceneComponent> ComponentClass;

	// Additional asset reference
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<class UObject> AdditionalAsset;

	// Name of the socket to attach to
	UPROPERTY(EditDefaultsOnly)
	FName SocketName;

	/** Get a list of assets to load before use*/
	TArray<FSoftObjectPath> GetAsyncAssets() const
	{
		TArray<FSoftObjectPath> Result;
		Result.Add(AdditionalAsset.ToSoftObjectPath());
		return Result;
	}
};

/** Interface wrapper */
UINTERFACE(MinimalAPI, Blueprintable)
class UNovaAdditionalComponentInterface : public UInterface
{
	GENERATED_BODY()
};

/** Base interface for additional components */
class INovaAdditionalComponentInterface
{
	GENERATED_BODY()

public:
	/** Configure the additional component */
	virtual void SetAdditionalAsset(TSoftObjectPtr<class UObject> AdditionalAsset) = 0;
};
