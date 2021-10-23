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
	Aft
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
	SoftCladding,
	Stealth
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
		if (!AdditionalAsset.IsNull())
		{
			Result.Add(AdditionalAsset.ToSoftObjectPath());
		}
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
	// Slot name in menus
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FText DisplayName;

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
	// Slot name in menus
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FText DisplayName;

	// Socket to attach to on the structure mesh
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName SocketName;

	// List of equipment types that can be mounted on this slot
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	TArray<ENovaEquipmentType> SupportedTypes;
};

/** Equipment slot group metadata */
USTRUCT()
struct FNovaEquipmentSlotGroup
{
	GENERATED_BODY()

public:
	// Socket names
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	TArray<FName> SocketNames;
};

/** Description of a main compartment asset */
UCLASS(ClassGroup = (Nova))
class UNovaCompartmentDescription : public UNovaTradableAssetDescription
{
	GENERATED_BODY()

public:
	/** Get a list of hull styles supported by this compartment */
	TArray<const UNovaHullDescription*> GetSupportedHulls() const;

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

	/** Get a list of equipment slot names grouped with the slot at Index */
	TArray<FName> GetGroupedEquipmentSlotsNames(int32 Index) const;

	/** Get a list of equipment slot metadata grouped with the slot at Index */
	TArray<const FNovaEquipmentSlot*> GetGroupedEquipmentSlots(int32 Index) const;

	/** Get a list of equipment slot indices grouped with the slot at Index */
	TArray<int32> GetGroupedEquipmentSlotsIndices(int32 Index) const;

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
	TSoftObjectPtr<class UStaticMesh> GetMainHull(const class UNovaHullDescription* Hull) const;

	/** Return the appropriate outer hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetOuterHull(const class UNovaHullDescription* Hull) const;

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

	/** Get the skirt between modules */
	TSoftObjectPtr<class UStaticMesh> GetBulkhead(
		const UNovaModuleDescription* ModuleDescription, ENovaBulkheadType Style, bool Forward) const;

	virtual struct FNovaAssetPreviewSettings GetPreviewSettings() const override;

	virtual void ConfigurePreviewActor(class AActor* Actor) const override;

	TArray<FText> GetDescription() const override;

public:
	// Main structural element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainStructure = nullptr;

	// Skirt structural element // TODO REVIEW
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

	// Decorative outer hull - soft mesh
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SoftHull = nullptr;

	// Decorative outer hull - rigid variant
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> RigidHull = nullptr;

	// Decorative outer hull (skirt) // TODO REVIEW
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterHull = nullptr;

	// Tank skirt - forward side of the module behind will be empty
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> TankSkirt = nullptr;

	// Cargo skirt - forward side of the module behind will be empty
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> CargoSkirt = nullptr;

	// Compartment mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Mass = 10;

	// Metadata for module slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaModuleSlot> ModuleSlots;

	// Metadata for equipment slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaEquipmentSlot> EquipmentSlots;

	// Groups of equipment slot that require identical equipment
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaEquipmentSlotGroup> EquipmentSlotsGroups;
};

/*----------------------------------------------------
    Hull and paint data assets
----------------------------------------------------*/

/** Description of a hull type */
UCLASS(ClassGroup = (Nova))
class UNovaHullDescription : public UNovaPreviableTradableAssetDescription
{
	GENERATED_BODY()

public:
	// Hull type
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaHullType Type;
};

/** Description of a generic paint asset */
UCLASS(ClassGroup = (Nova))
class UNovaPaintDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	UNovaPaintDescription() : PaintColor(FLinearColor::Black)
	{}

public:
	// Paint color
	UPROPERTY(Category = Paint, EditDefaultsOnly)
	FLinearColor PaintColor;
};

/** Description of a structural paint asset */
UCLASS(ClassGroup = (Nova))
class UNovaStructuralPaintDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	UNovaStructuralPaintDescription() : PaintColor(FLinearColor::Black), Unpainted(false)
	{}

public:
	// Paint color
	UPROPERTY(Category = Paint, EditDefaultsOnly)
	FLinearColor PaintColor;

	// Raw metal
	UPROPERTY(Category = Paint, EditDefaultsOnly)
	bool Unpainted;
};

/*----------------------------------------------------
    Module data assets
----------------------------------------------------*/

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaModuleDescription : public UNovaTradableAssetDescription
{
	GENERATED_BODY()

public:
	/** Get the appropriate bulkhead mesh */
	TSoftObjectPtr<class UStaticMesh> GetBulkhead(ENovaBulkheadType Style, bool Forward) const;

	virtual FNovaAssetPreviewSettings GetPreviewSettings() const override;

	virtual void ConfigurePreviewActor(class AActor* Actor) const override;

	TArray<FText> GetDescription() const override;

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

	// Whether the module allow a single connection for a train of identical modules
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool AllowCommonWiring = false;
};

/** Description of a propellant module */
UCLASS(ClassGroup = (Nova))
class UNovaPropellantModuleDescription : public UNovaModuleDescription
{
	GENERATED_BODY()

public:
	TArray<FText> GetDescription() const override;

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
	TArray<FText> GetDescription() const override;

public:
	// Cargo type that describes which kind of stuff it can carry
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaResourceType CargoType = ENovaResourceType::Bulk;

	// Cargo mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float CargoMass = 100;
};

/*----------------------------------------------------
    Equipment data assets
----------------------------------------------------*/

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEquipmentDescription : public UNovaTradableAssetDescription
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

	virtual FNovaAssetPreviewSettings GetPreviewSettings() const override;

	virtual void ConfigurePreviewActor(class AActor* Actor) const override;

	TArray<FText> GetDescription() const override;

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

	// Equipment pairing
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool RequiresPairing;
};

/** Description of an engine equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEngineDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()
public:
	virtual FNovaAssetPreviewSettings GetPreviewSettings() const override;

	TArray<FText> GetDescription() const override;

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
	// Thruster thrust in kN
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Thrust = 500;
};
