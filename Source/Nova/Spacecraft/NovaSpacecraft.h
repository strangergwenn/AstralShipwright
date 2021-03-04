// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftTypes.h"
#include "NovaSpacecraft.generated.h"

/*----------------------------------------------------
    Spacecraft description types
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

	// Metadata for module slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaModuleSlot> ModuleSlots;

	// Metadata for equipment slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaEquipmentSlot> EquipmentSlots;
};

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaModuleDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	/** Get the appropriate bulkhead mesh */
	TSoftObjectPtr<class UStaticMesh> GetBulkhead(ENovaBulkheadType Style, bool Forward) const
	{
		switch (Style)
		{
			case ENovaBulkheadType::None:
				return nullptr;
			case ENovaBulkheadType::Standard:
				return Forward ? ForwardBulkhead : AftBulkhead;
			case ENovaBulkheadType::Skirt:
				return Forward ? nullptr : SkirtBulkhead;
			case ENovaBulkheadType::Outer:
				return Forward ? OuterForwardBulkhead : OuterAftBulkhead;
			default:
				return nullptr;
		}
	}

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

	// Whether the module needs tank piping
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool NeedsPiping = false;
};

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEquipmentDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	TSoftObjectPtr<class UObject> GetMesh() const
	{
		if (!SkeletalEquipment.IsNull())
		{
			return SkeletalEquipment;
		}
		else if (!StaticEquipment.IsNull())
		{
			return StaticEquipment;
		}
		else
		{
			return nullptr;
		}
	}

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

	// Equipment requirement
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaEquipmentType EquipmentType;
};

/*----------------------------------------------------
    Spacecraft data types
----------------------------------------------------*/

/** Compartment module data */
USTRUCT(Atomic)
struct FNovaCompartmentModule
{
	GENERATED_BODY()

	FNovaCompartmentModule();

	bool operator==(const FNovaCompartmentModule& Other) const;

	bool operator!=(const FNovaCompartmentModule& Other) const
	{
		return !operator==(Other);
	}

	UPROPERTY()
	const class UNovaModuleDescription* Description;

	UPROPERTY()
	ENovaBulkheadType ForwardBulkheadType;

	UPROPERTY()
	ENovaBulkheadType AftBulkheadType;

	UPROPERTY()
	ENovaSkirtPipingType SkirtPipingType;

	UPROPERTY()
	bool NeedsWiring;
};

/** Compartment data */
USTRUCT(Atomic)
struct FNovaCompartment
{
	GENERATED_BODY()

	FNovaCompartment();

	FNovaCompartment(const class UNovaCompartmentDescription* K);

	bool operator==(const FNovaCompartment& Other) const;

	bool operator!=(const FNovaCompartment& Other) const
	{
		return !operator==(Other);
	}

	/** Check if this structure represents a non-empty compartment */
	bool IsValid() const
	{
		return Description != nullptr;
	}

	/** Get the description of the module residing at a particular socket name */
	const UNovaModuleDescription* GetModuleBySocket(FName SocketName) const
	{
		if (Description)
		{
			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				if (Description->GetModuleSlot(ModuleIndex).SocketName == SocketName)
				{
					return Modules[ModuleIndex].Description;
				}
			}
		}
		return nullptr;
	}

	/** Get the description of the equipment residing at a particular socket name */
	const UNovaEquipmentDescription* GetEquipmentySocket(FName SocketName) const
	{
		if (Description)
		{
			for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
			{
				if (Description->GetEquipmentSlot(EquipmentIndex).SocketName == SocketName)
				{
					return Equipments[EquipmentIndex];
				}
			}
		}
		return nullptr;
	}

	UPROPERTY()
	const class UNovaCompartmentDescription* Description;

	UPROPERTY()
	bool NeedsOuterSkirt;

	UPROPERTY()
	bool NeedsMainPiping;

	UPROPERTY()
	bool NeedsMainWiring;

	UPROPERTY()
	ENovaHullType HullType;

	UPROPERTY()
	FNovaCompartmentModule Modules[ENovaConstants::MaxModuleCount];

	UPROPERTY()
	const class UNovaEquipmentDescription* Equipments[ENovaConstants::MaxEquipmentCount];
};

/** Full spacecraft data */
USTRUCT(Atomic)
struct FNovaSpacecraft
{
	GENERATED_BODY()

public:
	FNovaSpacecraft() : Identifier(FGuid::NewGuid())
	{}

	bool operator==(const FNovaSpacecraft& Other) const;

	bool operator!=(const FNovaSpacecraft& Other) const
	{
		return !operator==(Other);
	}

	/** Update bulkheads, pipes, wiring, based on the current state */
	void UpdateProceduralElements();

	/** Get a safe copy of this spacecraft without empty compartments */
	FNovaSpacecraft GetSafeCopy() const;

	/** Get a shared pointer copy of this spacecraft */
	TSharedPtr<FNovaSpacecraft> GetSharedCopy() const;

	/** Serialize the spacecraft */
	static void SerializeJson(TSharedPtr<FNovaSpacecraft>& This, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

public:
	// Compartment data
	UPROPERTY()
	TArray<FNovaCompartment> Compartments;

	// Unique ID
	UPROPERTY()
	FGuid Identifier;
};
