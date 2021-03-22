// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
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
	const UNovaModuleDescription* GetModuleBySocket(FName SocketName) const;

	/** Get the description of the equipment residing at a particular socket name */
	const UNovaEquipmentDescription* GetEquipmentySocket(FName SocketName) const;

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

/** Metrics of the spacecraft's propulsion system */
struct FNovaSpacecraftPropulsionMetrics
{
	FNovaSpacecraftPropulsionMetrics()
		: DryMass(0)
		, FuelMass(0)
		, CargoMass(0)
		, TotalMass(0)
		, SpecificImpulse(0)
		, Thrust(0)
		, FuelRate(0)
		, ExhaustVelocity(0)
		, TotalDeltaV(0)
		, TotalBurnTime(0)
	{}

	// Dry mass before fuel and cargo in T
	float DryMass;

	// Maximum fuel mass in T
	float FuelMass;

	// Maximum cargo mass in T
	float CargoMass;

	// Maximum total mass in T
	float TotalMass;

	// Specific impulse in m/s
	float SpecificImpulse;

	// Maximum thrust in N
	float Thrust;

	// Fuel mass rate in T/S
	float FuelRate;

	// Engine exhaust velocity in m/s
	float ExhaustVelocity;

	// Total capable Delta-V in m/s
	float TotalDeltaV;

	// Total capable engine burn time in s
	float TotalBurnTime;
};

/** Spacecraft class */
USTRUCT(Atomic)
struct FNovaSpacecraft : public FFastArraySerializerItem
{
	GENERATED_BODY()

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
	void Create()
	{
		Identifier = FGuid::NewGuid();
	}

	/** Update bulkheads, pipes, wiring, based on the current state */
	void UpdateProceduralElements();

	/** Update the spacecraft's metrics */
	void UpdatePropulsionMetrics();

	/** Get propulsion characteristics for this spacecraft */
	const FNovaSpacecraftPropulsionMetrics& GetPropulsionMetrics() const
	{
		return PropulsionMetrics;
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

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
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

	// Local state
	FNovaSpacecraftPropulsionMetrics PropulsionMetrics;
};
