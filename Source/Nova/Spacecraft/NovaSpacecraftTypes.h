// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Game/NovaGameTypes.h"
#include "NovaSpacecraftTypes.generated.h"

/*----------------------------------------------------
    Basic spacecraft types
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

/*----------------------------------------------------
    Compartment data asset
----------------------------------------------------*/

/** Module slot metadata */
USTRUCT()
struct FNovaModuleSlot
{
	GENERATED_BODY()

public:
	// Socket to attach to on the structure mesh
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName SocketName;

	// Whether to force a simple connection pipe on this slot when no module is used
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	bool ForceSkirtPiping;
};

/** Equipment slot metadata */
USTRUCT()
struct FNovaEquipmentSlot
{
	GENERATED_BODY()

public:
	// Socket to attach to on the structure mesh
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName SocketName;

	// List of equipment types that can be mounted on this slot
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	TArray<ENovaEquipmentType> SupportedTypes;
};

/** Description of a main compartment asset */
UCLASS(ClassGroup = (Nova))
class UNovaCompartmentDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	/** Get a list of hull styles supported by this compartment */
	TArray<ENovaHullType> GetSupportedHullTypes() const
	{
		TArray<ENovaHullType> Result;

		Result.Add(ENovaHullType::None);
		Result.Add(ENovaHullType::PlasticFabric);
		Result.Add(ENovaHullType::MetalFabric);

		return Result;
	}

	/** Get the module setup at this index, if it exists, by index */
	FNovaModuleSlot GetModuleSlot(int32 Index) const
	{
		return Index < ModuleSlots.Num() ? ModuleSlots[Index] : FNovaModuleSlot();
	}

	/** Get the equipment setup at this index, if it exists, by index */
	FNovaEquipmentSlot GetEquipmentSlot(int32 Index) const
	{
		return Index < EquipmentSlots.Num() ? EquipmentSlots[Index] : FNovaEquipmentSlot();
	}

	/** Return the appropriate main piping mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainPiping(bool Enabled) const
	{
		return Enabled ? MainPiping : nullptr;
	}

	/** Return the appropriate skirt piping mesh */
	TSoftObjectPtr<class UStaticMesh> GetSkirtPiping(ENovaSkirtPipingType Type) const
	{
		switch (Type)
		{
			default:
			case ENovaSkirtPipingType::None:
				return nullptr;
			case ENovaSkirtPipingType::Simple:
				return SimpleSkirtPiping;
			case ENovaSkirtPipingType::Connection:
				return ConnectionSkirtPiping;
		}
	}

	/** Return the appropriate main hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainHull(ENovaHullType Type) const
	{
		return Type != ENovaHullType::None ? MainHull : nullptr;
	}

	/** Return the appropriate outer hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetOuterHull(ENovaHullType Type) const
	{
		return Type != ENovaHullType::None ? OuterHull : nullptr;
	}

	/** Return the appropriate main wiring mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainWiring(bool Enabled) const
	{
		return Enabled ? MainWiring : nullptr;
	}

	/** Return the appropriate module-connection wiring mesh */
	TSoftObjectPtr<class UStaticMesh> GetConnectionWiring(bool Enabled) const
	{
		return Enabled ? ConnectionWiring : nullptr;
	}

public:
	// Main structural element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainStructure = nullptr;

	// Skirt structural element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterStructure = nullptr;

	// Main piping element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainPiping = nullptr;

	// Simple direct piping (skirt)
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SimpleSkirtPiping = nullptr;

	// Tank-connected piping (skirt)
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ConnectionSkirtPiping = nullptr;

	// Module-connected wiring
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainWiring = nullptr;

	// Module-connected wiring
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ConnectionWiring = nullptr;

	// Decorative outer hull
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainHull = nullptr;

	// Decorative outer hull (skirt)
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterHull = nullptr;

	// Compartment mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Mass = 10;

	// Metadata for module slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaModuleSlot> ModuleSlots;

	// Metadata for equipment slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaEquipmentSlot> EquipmentSlots;
};

/*----------------------------------------------------
    Module data assets
----------------------------------------------------*/

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaModuleDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	/** Get the appropriate bulkhead mesh */
	TSoftObjectPtr<class UStaticMesh> GetBulkhead(ENovaBulkheadType Style, bool Forward) const;

public:
	// Main module segment
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> Segment = nullptr;

	// Standard forward bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ForwardBulkhead = nullptr;

	// Standard aft bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> AftBulkhead = nullptr;

	// Skirt bulkhead - forward side of the module behind will be empty
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SkirtBulkhead = nullptr;

	// Outer-facing forward bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterForwardBulkhead = nullptr;

	// Outer-facing aft external bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterAftBulkhead = nullptr;

	// Module mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Mass = 30;

	// Whether the module needs tank piping
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool NeedsPiping = false;
};

/** Description of a propellant module */
UCLASS(ClassGroup = (Nova))
class UNovaPropellantModuleDescription : public UNovaModuleDescription
{
	GENERATED_BODY()

public:
	// Module propellant mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float PropellantMass = 400;
};

/** Description of a cargo module */
UCLASS(ClassGroup = (Nova))
class UNovaCargoModuleDescription : public UNovaModuleDescription
{
	GENERATED_BODY()

public:
	// Cargo mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float CargoMass = 100;
};

/*----------------------------------------------------
    Equipment data assets
----------------------------------------------------*/

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEquipmentDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	/** Get the mesh for this equipment */
	TSoftObjectPtr<UObject> GetMesh() const;

	virtual TArray<FSoftObjectPath> GetAsyncAssets() const override
	{
		TArray<FSoftObjectPath> Result = Super::GetAsyncAssets();
		Result.Append(AdditionalComponent.GetAsyncAssets());
		return Result;
	}

public:
	// Animated equipment variant
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class USkeletalMesh> SkeletalEquipment;

	// Equipment animation
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UAnimationAsset> SkeletalAnimation;

	// Static equipment variant
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> StaticEquipment;

	// Additional component
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	FNovaAdditionalComponent AdditionalComponent;

	// Equipment mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Mass = 1;

	// Equipment requirement
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaEquipmentType EquipmentType;
};

/** Description of an engine equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEngineDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:
	// Engine thrust in kN
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Thrust = 15000;

	// Specific impulse in m/s
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float SpecificImpulse = 400;
};

/** Description of an RCS thruster block */
UCLASS(ClassGroup = (Nova))
class UNovaThrusterDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:
};
