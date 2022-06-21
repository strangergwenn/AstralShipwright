// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Game/NovaGameTypes.h"
#include "NovaSpacecraftTypes.h"
#include "NovaSpacecraft.generated.h"

/*----------------------------------------------------
    Spacecraft sub-structures
----------------------------------------------------*/

/** Compartment cargo entry */
USTRUCT()
struct FNovaSpacecraftCargo
{
	GENERATED_BODY()

	FNovaSpacecraftCargo() : Resource(nullptr), Amount(0)
	{}

	UPROPERTY()
	const UNovaResource* Resource;

	UPROPERTY()
	float Amount;
};

/** Compartment module data */
USTRUCT()
struct FNovaCompartmentModule
{
	GENERATED_BODY()

	FNovaCompartmentModule();

	bool operator==(const FNovaCompartmentModule& Other) const
	{
		return Description == Other.Description;
	}

	bool operator!=(const FNovaCompartmentModule& Other) const
	{
		return !operator==(Other);
	}

	UPROPERTY()
	const class UNovaModuleDescription* Description;

	UPROPERTY()
	FNovaSpacecraftCargo Cargo;

	ENovaBulkheadType ForwardBulkheadType;

	ENovaBulkheadType AftBulkheadType;

	ENovaSkirtPipingType SkirtPipingType;

	bool NeedsConnectionWiring;
	bool NeedsCollectorPiping;
	bool NeedsDome;
	bool NeedsSkirt;
};

/** Compartment data */
USTRUCT()
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

	/** Get the module data for the module residing at a particular socket name */
	const FNovaCompartmentModule* GetModuleDataBySocket(FName SocketName, int32& FoundModuleIndex) const;

	/** Get the description of the module residing at a particular socket name */
	const UNovaModuleDescription* GetModuleBySocket(FName SocketName) const;

	/** Get the description of the equipment residing at a particular socket name */
	const UNovaEquipmentDescription* GetEquipmentySocket(FName SocketName) const;

	/** Check if this module has equipment on the a forward/aft slot */
	bool HasForwardOrAftEquipment() const;

	/** Get the cargo for a particular type */
	FNovaSpacecraftCargo& GetCargo(int32 ModuleIndex)
	{
		return Modules[ModuleIndex].Cargo;
	}

	/** Get the cargo for a module */
	const FNovaSpacecraftCargo& GetCargo(int32 ModuleIndex) const
	{
		return Modules[ModuleIndex].Cargo;
	}

	/** Get the cargo type for a module */
	ENovaResourceType GetCargoType(int32 ModuleIndex) const;

	/** Get the cargo capacity  */
	float GetCargoCapacity(int32 ModuleIndex) const;

	/** Get the cargo capacity for a particular type */
	float GetCargoCapacity(int32 ModuleIndex, ENovaResourceType Type) const;

	/** Get the current cargo mass */
	float GetCargoMass(int32 ModuleIndex) const;

	/** Get the amount of cargo mass used by one resource */
	float GetCargoMass(int32 ModuleIndex, const class UNovaResource* Resource) const;

	/** Get the amount of cargo mass available for one resource */
	float GetAvailableCargoMass(int32 ModuleIndex, const class UNovaResource* Resource) const;

	/** Check whether it's possible to apply a delta to this cargo - only checks MassDelta for sign, not value */
	bool CanModifyCargo(int32 ModuleIndex, const class UNovaResource* Resource, float MassDelta) const;

	/** Add a (possibly negative) amount of resources to the cargo */
	void ModifyCargo(int32 ModuleIndex, const class UNovaResource* Resource, float& MassDelta);

	UPROPERTY()
	const class UNovaCompartmentDescription* Description;

	UPROPERTY()
	const UNovaHullDescription* HullType;

	UPROPERTY()
	FNovaCompartmentModule Modules[ENovaConstants::MaxModuleCount];

	UPROPERTY()
	const class UNovaEquipmentDescription* Equipment[ENovaConstants::MaxEquipmentCount];
};

/** Spacecraft customization data */
USTRUCT()
struct FNovaSpacecraftCustomization
{
	GENERATED_BODY()

	FNovaSpacecraftCustomization()
		: DirtyIntensity(0.0f), StructuralPaint(nullptr), HullPaint(nullptr), DetailPaint(nullptr), Emblem(nullptr), EnableHullPaint(false)
	{}

	/** Initialize the customization data */
	void Create();

	bool operator==(const FNovaSpacecraftCustomization& Other) const
	{
		return DirtyIntensity == Other.DirtyIntensity && StructuralPaint == Other.StructuralPaint && HullPaint == Other.HullPaint &&
		       DetailPaint == Other.DetailPaint && Emblem == Other.Emblem && EnableHullPaint == Other.EnableHullPaint;
	}

	bool operator!=(const FNovaSpacecraftCustomization& Other) const
	{
		return !operator==(Other);
	}

	UPROPERTY()
	float DirtyIntensity;

	UPROPERTY()
	const UNovaPaintDescription* StructuralPaint;

	UPROPERTY()
	const UNovaPaintDescription* HullPaint;

	UPROPERTY()
	const UNovaPaintDescription* DetailPaint;

	UPROPERTY()
	const UNovaEmblemDescription* Emblem;

	UPROPERTY()
	bool EnableHullPaint;
};

/** Metrics for a spacecraft compartment */
struct FNovaSpacecraftCompartmentMetrics : public INeutronDescriptibleInterface
{
	FNovaSpacecraftCompartmentMetrics()
		: ModuleCount(0)
		, EquipmentCount(0)
		, DryMass(0)
		, PropellantMassCapacity(0)
		, CargoMassCapacity(0)
		, Thrust(0)
		, TotalEngineISPTimesThrust(0)
	{}

	FNovaSpacecraftCompartmentMetrics(const struct FNovaSpacecraft& Spacecraft, int32 CompartmentIndex);

	TArray<FText> GetDescription() const override;

public:

	int32 ModuleCount;
	int32 EquipmentCount;
	float DryMass;
	float PropellantMassCapacity;
	float CargoMassCapacity;
	float Thrust;
	float TotalEngineISPTimesThrust;
};

/** Metrics of the spacecraft's propulsion system */
struct FNovaSpacecraftPropulsionMetrics
{
	FNovaSpacecraftPropulsionMetrics()
		: DryMass(0)
		, PropellantMassCapacity(0)
		, CargoMassCapacity(0)
		, MaximumMass(0)
		, SpecificImpulse(0)
		, EngineThrust(0)
		, PropellantRate(0)
		, ExhaustVelocity(0)
		, MaximumDeltaV(0)
		, MaximumBurnTime(0)
		, ThrusterThrust(0)
	{}

	/** Get the duration & mass of propellant spent in T for a maneuver */
	FNovaTime GetManeuverDurationAndPropellantUsed(float DeltaV, float CurrentCargoMass, float& CurrentPropellantMass) const
	{
		NCHECK(EngineThrust > 0);
		NCHECK(ExhaustVelocity > 0);

		float Duration = (((DryMass + CurrentCargoMass + CurrentPropellantMass) * 1000.0f * ExhaustVelocity) / (EngineThrust * 1000.0f)) *
		                 (1.0f - exp(-abs(DeltaV) / ExhaustVelocity));
		float PropellantUsed = PropellantRate * Duration;

		CurrentPropellantMass -= PropellantUsed;

		return FNovaTime::FromSeconds(Duration);
	}

	/** Get the remaining delta-v in m/s */
	float GetRemainingDeltaV(float CurrentCargoMass, float CurrentPropellantMass) const
	{
		if (DryMass > 0 && ExhaustVelocity > 0)
		{
			const float CurrentTotalMass = DryMass + CurrentCargoMass + CurrentPropellantMass;

			return ExhaustVelocity * log(CurrentTotalMass / DryMass);
		}
		else
		{
			return 0.0f;
		}
	}

	// Dry mass before propellants and cargo in T
	float DryMass;

	// Maximum propellant mass in T
	float PropellantMassCapacity;

	// Maximum cargo mass in T
	float CargoMassCapacity;

	// Maximum total mass in T
	float MaximumMass;

	// Specific impulse in m/s
	float SpecificImpulse;

	// Full-power thrust in N
	float EngineThrust;

	// Propellant mass rate in T/s
	float PropellantRate;

	// Engine exhaust velocity in m/s
	float ExhaustVelocity;

	// Total capable delta-v in m/s
	float MaximumDeltaV;

	// Total capable engine burn time in s
	float MaximumBurnTime;

	// Spacecraft thruster force in kN
	float ThrusterThrust;
};

/** Types of module groups */
enum class ENovaModuleGroupType : uint8
{
	Hatch,
	Propellant
};

/** Module group data for a single compartment */
struct FNovaModuleGroupCompartment
{
	FNovaModuleGroupCompartment(int32 CI, int32 MI) : CompartmentIndex(CI)
	{
		ModuleIndices.Add(MI);
	}

	int32         CompartmentIndex;
	TArray<int32> ModuleIndices;
	TArray<FName> LinkedEquipments;
};

/** Group of spacecraft modules with common bulkheads */
struct FNovaModuleGroup
{
	FNovaModuleGroup() : HasHatch(false)
	{}

	TArray<FNovaModuleGroupCompartment> Compartments;
	ENovaModuleGroupType                Type;
	bool                                HasHatch;
};

/** Spacecraft upgrade cost result */
struct FNovaSpacecraftUpgradeCost
{
	FNovaSpacecraftUpgradeCost() : UpgradeCost(0), ResaleGain(0), PaintCost(0), TotalChangeCost(0), TotalCost(0)
	{}

	FNovaCredits UpgradeCost;
	FNovaCredits ResaleGain;
	FNovaCredits PaintCost;
	FNovaCredits TotalChangeCost;
	FNovaCredits TotalCost;
};

/*----------------------------------------------------
    Spacecraft implementation
----------------------------------------------------*/

/** Spacecraft class */
USTRUCT()
struct FNovaSpacecraft : public FFastArraySerializerItem
{
	GENERATED_BODY()

	friend struct FNovaSpacecraftCompartmentMetrics;

public:

	/*----------------------------------------------------
	    Constructor & operators
	----------------------------------------------------*/

	FNovaSpacecraft() : Identifier(0, 0, 0, 0), SpacecraftClass(nullptr), PropellantMassAtLaunch(0)
	{}

	bool operator==(const FNovaSpacecraft& Other) const;

	bool operator!=(const FNovaSpacecraft& Other) const
	{
		return !operator==(Other);
	}

	/*----------------------------------------------------
	    Basic interface
	----------------------------------------------------*/

	/** Create a new spacecraft */
	void Create(FString SpacecraftName)
	{
		Identifier = FGuid::NewGuid();
		Name       = SpacecraftName;

		Customization.Create();
	}

	/** Get the spacecraft validity */
	bool IsValid(FText* Details = nullptr) const;

	/** Compute the cost structure of an upgrade */
	FNovaSpacecraftUpgradeCost GetUpgradeCost(const class ANovaGameState* GameState, const FNovaSpacecraft* Other) const;

	/** Get a safe copy of this spacecraft without empty compartments */
	FNovaSpacecraft GetSafeCopy() const
	{
		FNovaSpacecraft NewSpacecraft = *this;

		NewSpacecraft.Compartments.Empty();
		for (const FNovaCompartment& Compartment : Compartments)
		{
			if (Compartment.Description != nullptr)
			{
				NewSpacecraft.Compartments.Add(Compartment);
			}
		}

		return NewSpacecraft;
	}

	/** Get a shared pointer copy of this spacecraft */
	TSharedPtr<FNovaSpacecraft> GetSharedCopy() const
	{
		TSharedPtr<FNovaSpacecraft> NewSpacecraft = MakeShared<FNovaSpacecraft>();
		*NewSpacecraft                            = *this;
		return NewSpacecraft;
	}

	/** Get this spacecraft's name */
	FText GetName() const
	{
		return FText::FromString(Name);
	}

	/** Get this spacecraft's classification */
	FText GetClassification() const;

	/** Get the customization data */
	const struct FNovaSpacecraftCustomization& GetCustomization() const
	{
		return Customization;
	}

	/*----------------------------------------------------
	    Propulsion metrics & cargo hold
	----------------------------------------------------*/

	/** Get propulsion characteristics for this spacecraft */
	const FNovaSpacecraftPropulsionMetrics& GetPropulsionMetrics() const
	{
		return PropulsionMetrics;
	}

	/** Get module groups for this spacecraft */
	const TArray<FNovaModuleGroup>& GetModuleGroups() const
	{
		return ModuleGroups;
	}

	/** Update bulkheads, pipes, wiring, based on the current state */
	void UpdateProceduralElements();

	/** Update the spacecraft's metrics */
	void UpdatePropulsionMetrics();

	/** Update module groups*/
	void UpdateModuleGroups();

	/** Reset the propellant amount */
	void SetPropellantMass(float Amount)
	{
		PropellantMassAtLaunch = Amount;
	}

	/** Get the current cargo mass */
	float GetCurrentCargoMass() const
	{
		float CargoMass = 0;

		for (const FNovaCompartment& Compartment : Compartments)
		{
			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				CargoMass += Compartment.GetCargoMass(ModuleIndex);
			}
		}

		return CargoMass;
	}

	/** Get the cargo hold for a particular type */
	FNovaSpacecraftCargo& GetCargo(int32 CompartmentIndex, int32 ModuleIndex)
	{
		NCHECK(CompartmentIndex >= 0 && CompartmentIndex < Compartments.Num());
		return Compartments[CompartmentIndex].GetCargo(ModuleIndex);
	}

	/** Get the cargo capacity for a particular type, across the ship or in a specific compartment */
	float GetCargoCapacity(ENovaResourceType Type, int32 CompartmentIndex = INDEX_NONE, int32 ModuleIndex = INDEX_NONE) const;

	/** Get the amount of cargo mass used by one resource, across the ship or in a specific compartment */
	float GetCargoMass(const class UNovaResource* Resource, int32 CompartmentIndex = INDEX_NONE, int32 ModuleIndex = INDEX_NONE) const;

	/** Get the amount of cargo mass available for one resource, across the ship or in a specific compartment */
	float GetAvailableCargoMass(
		const class UNovaResource* Resource, int32 CompartmentIndex = INDEX_NONE, int32 ModuleIndex = INDEX_NONE) const;

	/** Get all resources in cargo */
	TArray<const class UNovaResource*> GetOwnedResources() const;

	/** Add a (possibly negative) amount of resources to the spacecraft, across the ship or in a specific compartment */
	bool ModifyCargo(
		const class UNovaResource* Resource, float MassDelta, int32 CompartmentIndex = INDEX_NONE, int32 ModuleIndex = INDEX_NONE);

	/*----------------------------------------------------
	    UI helpers
	----------------------------------------------------*/

	/** Get a list of compartment kits that can be added at a (new) index */
	TArray<const class UNovaCompartmentDescription*> GetCompatibleCompartments(int32 CompartmentIndex) const;

	/** Get a list of compatible modules that can be added at a compartment index, and a module slot index */
	TArray<const class UNovaModuleDescription*> GetCompatibleModules(int32 CompartmentIndex, int32 SlotIndex) const;

	/** Get a list of compatible equipment that can be added at a compartment index, and an equipment slot index */
	TArray<const class UNovaEquipmentDescription*> GetCompatibleEquipment(int32 InCompartmentIndexdex, int32 SlotIndex) const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:

	/** Check whether this is the first (head) compartment */
	bool IsFirstCompartment(int32 CompartmentIndex) const;

	/** Check whether this is the last (engine) compartment */
	bool IsLastCompartment(int32 CompartmentIndex) const;

	/** Check whether the module at CompartmentIndex.ModuleIndex has a matching clone in front of it */
	bool IsSameModuleInPreviousCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		return IsAnyModuleInPreviousCompartment(CompartmentIndex, ModuleIndex, true);
	}

	/** Check whether the module at CompartmentIndex.ModuleIndex has a matching clone behind it */
	bool IsSameModuleInNextCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		return IsAnyModuleInNextCompartment(CompartmentIndex, ModuleIndex, true);
	}

	/** Check whether the module at CompartmentIndex.ModuleIndex has any other module in front of it */
	bool IsAnyModuleInPreviousCompartment(int32 CompartmentIndex, int32 ModuleIndex, bool RequireSameType = false) const
	{
		return GetModuleInPreviousCompartment(CompartmentIndex, ModuleIndex, RequireSameType) != nullptr;
	}

	/** Check whether the module at CompartmentIndex.ModuleIndex has any other module behind it */
	bool IsAnyModuleInNextCompartment(int32 CompartmentIndex, int32 ModuleIndex, bool RequireSameType = false) const
	{
		return GetModuleInNextCompartment(CompartmentIndex, ModuleIndex, RequireSameType) != nullptr;
	}

	/** Fetch the module in front of the one at CompartmentIndex.ModuleIndex if any */
	const UNovaModuleDescription* GetModuleInPreviousCompartment(
		int32 CompartmentIndex, int32 ModuleIndex, bool RequireSameType = false) const;

	/** Fetch the module behind the one at CompartmentIndex.ModuleIndex if any */
	const UNovaModuleDescription* GetModuleInNextCompartment(int32 CompartmentIndex, int32 ModuleIndex, bool RequireSameType = false) const;

	/** Check whether the module at CompartmentIndex.ModuleIndex has another hatch-needing module behind it */
	bool IsSameKindModuleInPreviousCompartment(
		int32 CompartmentIndex, int32 ModuleIndex, ENovaModuleGroupType Type, int32& FoundCompartmentIndex, int32& FoundModuleIndex) const;

	/** Get the module group classification for this module */
	ENovaModuleGroupType GetModuleType(const UNovaModuleDescription* Module) const;

public:

	// Compartment data
	UPROPERTY()
	TArray<FNovaCompartment> Compartments;

	// Unique ID
	UPROPERTY()
	FGuid Identifier;

	// Name
	UPROPERTY()
	FString Name;

	// Customization data
	UPROPERTY()
	FNovaSpacecraftCustomization Customization;

	// Optional spacecraft class for AI ships
	const class UNovaAISpacecraftDescription* SpacecraftClass;

	// Propellant mass while docked - serves as persistent storage
	// The real-time value while flying is handled by the propellant system
	UPROPERTY()
	float PropellantMassAtLaunch;

	// Local state
	FNovaSpacecraftPropulsionMetrics PropulsionMetrics;
	TArray<FNovaModuleGroup>         ModuleGroups;
};
