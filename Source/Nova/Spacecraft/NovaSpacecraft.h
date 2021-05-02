// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Game/NovaGameTypes.h"
#include "NovaSpacecraftTypes.h"
#include "NovaSpacecraft.generated.h"

/** Compartment module data */
USTRUCT(Atomic)
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

	ENovaBulkheadType ForwardBulkheadType;

	ENovaBulkheadType AftBulkheadType;

	ENovaSkirtPipingType SkirtPipingType;

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
	const UNovaModuleDescription* GetModuleBySocket(FName SocketName) const;

	/** Get the description of the equipment residing at a particular socket name */
	const UNovaEquipmentDescription* GetEquipmentySocket(FName SocketName) const;

	UPROPERTY()
	const class UNovaCompartmentDescription* Description;

	UPROPERTY()
	ENovaHullType HullType;

	UPROPERTY()
	FNovaCompartmentModule Modules[ENovaConstants::MaxModuleCount];

	UPROPERTY()
	const class UNovaEquipmentDescription* Equipments[ENovaConstants::MaxEquipmentCount];

	bool NeedsOuterSkirt;

	bool NeedsMainPiping;

	bool NeedsMainWiring;
};

/** Metrics for a spacecraft compartment */
struct FNovaSpacecraftCompartmentMetrics : public INovaDescriptibleInterface
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
		, Thrust(0)
		, PropellantRate(0)
		, ExhaustVelocity(0)
		, MaximumDeltaV(0)
		, MaximumBurnTime(0)
	{}

	/** Get the worst-case acceleration for this spacecraft in m/s² */
	float GetLowestAcceleration() const
	{
		return Thrust / (DryMass + PropellantMassCapacity + CargoMassCapacity);
	}

	/** Get the duration & mass of propellant spent in T for a maneuver */
	FNovaTime GetManeuverDurationAndPropellantUsed(const float DeltaV, float& CurrentPropellantMass) const
	{
		NCHECK(Thrust > 0);
		NCHECK(ExhaustVelocity > 0);

		float Duration = (((DryMass + CargoMassCapacity + CurrentPropellantMass) * 1000.0f * ExhaustVelocity) / (Thrust * 1000.0f)) *
						 (1.0f - exp(-abs(DeltaV) / ExhaustVelocity)) / 60.0f;
		float PropellantUsed = PropellantRate * Duration;

		CurrentPropellantMass -= PropellantUsed;
		NCHECK(CurrentPropellantMass > 0);

		return FNovaTime::FromMinutes(Duration);
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
	float Thrust;

	// Propellant mass rate in T/s
	float PropellantRate;

	// Engine exhaust velocity in m/s
	float ExhaustVelocity;

	// Total capable delta-v in m/s
	float MaximumDeltaV;

	// Total capable engine burn time in s
	float MaximumBurnTime;
};

/** Spacecraft launch parameters */
USTRUCT(Atomic)
struct FNovaSpacecraftSystemState
{
	GENERATED_BODY()

	UPROPERTY()
	float InitialPropellantMass;
};

/** Spacecraft class */
USTRUCT(Atomic)
struct FNovaSpacecraft : public FFastArraySerializerItem
{
	GENERATED_BODY()

	friend struct FNovaSpacecraftCompartmentMetrics;

public:
	/*----------------------------------------------------
	    Constructor & operators
	----------------------------------------------------*/

	FNovaSpacecraft() : Identifier(0, 0, 0, 0)
	{}

	bool operator==(const FNovaSpacecraft& Other) const;

	bool operator!=(const FNovaSpacecraft& Other) const
	{
		return !operator==(Other);
	}

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	/** Create a new spacecraft */
	void Create(FString SpacecraftName)
	{
		Identifier = FGuid::NewGuid();
		Name       = SpacecraftName;
	}

	/** Get the spacecraft validity */
	bool IsValid(FText* Details) const;

	/** Get this spacecraft's name */
	FText GetName() const
	{
		return FText::FromString(Name);
	}

	/** Get this spacecraft's classification */
	FText GetClassification() const;

	/** Get propulsion characteristics for this spacecraft */
	const FNovaSpacecraftPropulsionMetrics& GetPropulsionMetrics() const
	{
		return PropulsionMetrics;
	}

	/** Update bulkheads, pipes, wiring, based on the current state */
	void UpdateProceduralElements();

	/** Update the spacecraft's metrics */
	void UpdatePropulsionMetrics();

	/** Reset completely the propellant amount to the spacecraft's maximum */
	void RefillPropellant()
	{
		SystemState.InitialPropellantMass = PropulsionMetrics.PropellantMassCapacity;
	}

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

	/** Serialize the spacecraft */
	static void SerializeJson(TSharedPtr<FNovaSpacecraft>& This, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/** Get a list of compartment kits that can be added at a (new) index */
	TArray<const class UNovaCompartmentDescription*> GetCompatibleCompartments(int32 CompartmentIndex) const;

	/** Get a list of compatible modules that can be added at a compartment index, and a module slot index */
	TArray<const class UNovaModuleDescription*> GetCompatibleModules(int32 CompartmentIndex, int32 SlotIndex) const;

	/** Get a list of compatible equipments that can be added at a compartment index, and an equipment slot index */
	TArray<const class UNovaEquipmentDescription*> GetCompatibleEquipments(int32 InCompartmentIndexdex, int32 SlotIndex) const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Check whether this is the first (head) compartment */
	bool IsFirstCompartment(int32 CompartmentIndex) const;

	/** Check whether this is the last (engine) compartment */
	bool IsLastCompartment(int32 CompartmentIndex) const;

	/** Check whether the module at CompartmentIndex.ModuleIndex has a matching clone behind it */
	bool IsSameModuleInPreviousCompartment(int32 CompartmentIndex, int32 ModuleIndex) const;

	/** Check whether the module at CompartmentIndex.ModuleIndex has a matching clone in front of it */
	bool IsSameModuleInNextCompartment(int32 CompartmentIndex, int32 ModuleIndex) const;

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

	// System state
	UPROPERTY()
	FNovaSpacecraftSystemState SystemState;

	// Local state
	FNovaSpacecraftPropulsionMetrics PropulsionMetrics;
};
