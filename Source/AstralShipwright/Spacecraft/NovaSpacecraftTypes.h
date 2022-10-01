// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Game/NovaGameTypes.h"
#include "NovaSpacecraftTypes.generated.h"

/*----------------------------------------------------
    Basic spacecraft types
----------------------------------------------------*/

/** Equipment requirement types */
UENUM()
enum class ENovaEquipmentType : uint8
{
	Standard,       // Equipment that may transit cargo, humans, propellant
	Unconnected,    // Simplified equipment that only needs a few wires
	Forward,        // Large equipment forward of and coaxial to the compartment
	Aft             // Large equipment aft of the compartment
};

/** Equipment filter*/
UENUM()
enum class ENovaEquipmentCategory : uint8
{
	Standard,
	Propulsion,
	Power,
	Accessory
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
	Connection,
	ShortConnection
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

	FSoftObjectPath              Asset;
	ENovaAssemblyElementType     Type;
	class INeutronMeshInterface* Mesh = nullptr;
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
	virtual void SetAdditionalAsset(TSoftObjectPtr<class UObject> AdditionalAsset){};
};

/*----------------------------------------------------
    Compartment data asset
----------------------------------------------------*/

/** Module slot metadata */
USTRUCT()
struct FNovaModuleSlot
{
	GENERATED_BODY()

	FNovaModuleSlot() : ForceSkirtPiping(false)
	{}

public:

	// Slot name in menus
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FText DisplayName;

	// Socket to attach to on the structure mesh
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName SocketName;

	// Force a simple pipe on this slot when no module is used and disable collector piping
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	bool ForceSkirtPiping;

	// Connected equipment slot names
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	TArray<FName> LinkedEquipments;

	// Skirt-supported equipment slot names
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	TArray<FName> SupportedEquipments;
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

	// Socket to attach to on the structure mesh, specifically for the forward node
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName ForwardSocketName;

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
			case ENovaSkirtPipingType::ShortConnection:
				return ShortConnectionSkirtPiping;
		}
	}

	/** Return the appropriate main hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainHull(const class UNovaHullDescription* Hull) const;

	/** Return the appropriate forward dome hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetDomeHull(const class UNovaHullDescription* Hull) const;

	/** Return the appropriate skirt hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetSkirtHull(const class UNovaHullDescription* Hull) const;

	/** Return the appropriate module-connection wiring mesh */
	TSoftObjectPtr<class UStaticMesh> GetConnectionWiring(bool Enabled) const
	{
		return Enabled ? ConnectionWiring : nullptr;
	}

	/** Get the skirt between modules */
	TSoftObjectPtr<class UStaticMesh> GetBulkhead(
		const UNovaModuleDescription* ModuleDescription, ENovaBulkheadType Style, bool Forward) const;

	virtual struct FNeutronAssetPreviewSettings GetPreviewSettings() const override;

	virtual void ConfigurePreviewActor(class AActor* Actor) const override;

	TArray<FText> GetDescription() const override;

public:

	// Main structural element
	UPROPERTY(Category = Structure, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainStructure = nullptr;

	// Skirt structural element
	UPROPERTY(Category = Structure, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SkirtStructure = nullptr;

	// Tank skirt - forward side of the module behind will be empty
	UPROPERTY(Category = Structure, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> TankSkirt = nullptr;

	// Cargo skirt - forward side of the module behind will be empty
	UPROPERTY(Category = Structure, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> CargoSkirt = nullptr;

	// Main piping element
	UPROPERTY(Category = Piping, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainPiping = nullptr;

	// Simple direct piping (skirt)
	UPROPERTY(Category = Piping, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SimpleSkirtPiping = nullptr;

	// Tank-connected piping (skirt)
	UPROPERTY(Category = Piping, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ConnectionSkirtPiping = nullptr;

	// Tank-connected piping (skirt, shortened)
	UPROPERTY(Category = Piping, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ShortConnectionSkirtPiping = nullptr;

	// Side collector piping element
	UPROPERTY(Category = Piping, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> CollectorPiping = nullptr;

	// Module-connected wiring
	UPROPERTY(Category = Wiring, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainWiring = nullptr;

	// Module-connected wiring
	UPROPERTY(Category = Wiring, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ConnectionWiring = nullptr;

	// Decorative outer hull - soft padding
	UPROPERTY(Category = Hull, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SoftHull = nullptr;

	// Decorative outer hull - rigid variant
	UPROPERTY(Category = Hull, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> RigidHull = nullptr;

	// Decorative outer hull - soft padding (forward dome)
	UPROPERTY(Category = Hull, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> DomeSoftHull = nullptr;

	// Decorative outer hull - rigid variant (forward dome)
	UPROPERTY(Category = Hull, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> DomeRigidHull = nullptr;

	// Decorative outer hull - soft padding (skirt)
	UPROPERTY(Category = Hull, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SkirtSoftHull = nullptr;

	// Decorative outer hull - rigid variant (skirt)
	UPROPERTY(Category = Hull, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SkirtRigidHull = nullptr;

	// Is this the most forward of compartment
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool IsForwardCompartment;

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
class UNovaPaintDescription : public UNeutronAssetDescription
{
	GENERATED_BODY()

public:

	UNovaPaintDescription() : PaintColor(FLinearColor::Black)
	{}

	virtual bool operator<(const UNovaPaintDescription& Other) const
	{
		return PaintColor.LinearRGBToHSV().R < Other.PaintColor.LinearRGBToHSV().R;
	}

public:

	// Paint color
	UPROPERTY(Category = Paint, EditDefaultsOnly)
	FLinearColor PaintColor;
};

/** Description of an emblem */
UCLASS(ClassGroup = (Nova))
class UNovaEmblemDescription : public UNeutronAssetDescription
{
	GENERATED_BODY()

public:

	UNovaEmblemDescription() : Image(nullptr)
	{}

	virtual bool operator<(const UNovaEmblemDescription& Other) const
	{
		return Image && Other.Image && Image->GetName() < Other.Image->GetName();
	}

public:

	// Emblem brush
	UPROPERTY(Category = Paint, EditDefaultsOnly)
	FSlateBrush Brush;

	// Emblem RGBA image
	UPROPERTY(Category = Paint, EditDefaultsOnly)
	class UTexture2D* Image;
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

	virtual FNeutronAssetPreviewSettings GetPreviewSettings() const override;

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

	// Display details
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FText Description;

	// Module mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Mass = 30;

	// Crew effect - positive for crew, negative for crew attendance
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 CrewEffect;

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

	// Cargo mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float CargoMass = 100;
};

/** Description of a processing module */
UCLASS(ClassGroup = (Nova))
class UNovaProcessingModuleDescription : public UNovaModuleDescription
{
	GENERATED_BODY()

public:

	TArray<FText> GetDescription() const override;

public:

	// Resource inputs
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<const class UNovaResource*> Inputs;

	// Resource outputs
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<const class UNovaResource*> Outputs;

	// Resource processing rate in seconds, applied to all input & outputs
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float ProcessingRate = 1;

	// Power drain in kW
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 Power = 0;

	// Energy capacity in kWH
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 Capacity = 0;
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

	virtual FNeutronAssetPreviewSettings GetPreviewSettings() const override;

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

	// Display details
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FText Description;

	// Equipment mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float Mass = 1;

	// Equipment requirement
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaEquipmentType EquipmentType = ENovaEquipmentType::Standard;

	// Equipment category
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaEquipmentCategory EquipmentCategory = ENovaEquipmentCategory::Standard;

	// Equipment pairing
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool RequiresPairing = false;

	// Crew effect - positive for crew, negative for crew attendance
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 CrewEffect;
};

/** Description of an engine equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEngineDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()
public:

	virtual FNeutronAssetPreviewSettings GetPreviewSettings() const override;

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

/** Description of a mining equipment */
UCLASS(ClassGroup = (Nova))
class UNovaMiningEquipmentDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:

	TArray<FText> GetDescription() const override;

	virtual FNeutronAssetPreviewSettings GetPreviewSettings() const override;

public:

	// Resource extraction rate in seconds
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float ExtractionRate = 1;

	// Power drain in kW
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 Power = 0;
};

/** Description of a radio mast */
UCLASS(ClassGroup = (Nova))
class UNovaRadioMastDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:

	TArray<FText> GetDescription() const override;

public:

	// Power drain in kW
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 Power = 0;
};

/** Description of an electrical component */
UCLASS(ClassGroup = (Nova))
class UNovaPowerEquipmentDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:

	TArray<FText> GetDescription() const override;

public:

	// Power generation in kW
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 Power = 0;

	// Energy capacity in kWh
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	int32 Capacity = 0;
};

/** Mining rig attach point */
UCLASS(ClassGroup = (Nova))
class UNovaSpacecraftMiningRigComponent
	: public USceneComponent
	, public INovaAdditionalComponentInterface
{
	GENERATED_BODY()
};

/** Description of a hatch equipment */
UCLASS(ClassGroup = (Nova))
class UNovaHatchDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:

	// Life support
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool IsHabitat = false;
};

/** Description of a propellant tank equipment */
UCLASS(ClassGroup = (Nova))
class UNovaPropellantEquipmentDescription : public UNovaEquipmentDescription
{
	GENERATED_BODY()

public:

	TArray<FText> GetDescription() const override;

public:

	// Module propellant mass in T
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float PropellantMass = 50;
};
