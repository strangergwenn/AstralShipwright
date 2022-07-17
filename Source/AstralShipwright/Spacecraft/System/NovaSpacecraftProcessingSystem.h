// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaSpacecraftSystemInterface.h"
#include "Game/NovaOrbitalSimulationComponent.h"
#include "Components/SceneComponent.h"

#include "NovaSpacecraftProcessingSystem.generated.h"

/** Module state */
UENUM()
enum class ENovaSpacecraftProcessingSystemStatus : uint8
{
	Stopped,
	Processing,
	Blocked,
	Docked
};

/** Replicated compartment cargo */
USTRUCT()
struct FNovaSpacecraftProcessingSystemCompartmentCargo
{
	GENERATED_BODY();

	UPROPERTY()
	TArray<FNovaSpacecraftCargo> Cargo;
};

/** Processing state for a chain */
USTRUCT()
struct FNovaSpacecraftProcessingSystemChainState
{
	GENERATED_BODY();

	FNovaSpacecraftProcessingSystemChainState() : Status(ENovaSpacecraftProcessingSystemStatus::Stopped)
	{}

	// Replicated status
	UPROPERTY()
	ENovaSpacecraftProcessingSystemStatus Status;

	// Replicated inputs
	UPROPERTY()
	TArray<const class UNovaResource*> Inputs;

	// Replicated outputs
	UPROPERTY()
	TArray<const class UNovaResource*> Outputs;

	// Replicated rate
	UPROPERTY()
	float ProcessingRate;

	// Local data
	TArray<const class UNovaProcessingModuleDescription*> Modules;
};

/** Processing state for a full module group */
USTRUCT()
struct FNovaSpacecraftProcessingSystemGroupState
{
	GENERATED_BODY();

	UPROPERTY()
	TArray<FNovaSpacecraftProcessingSystemChainState> Chains;
};

/** Resource processing system that transforms resources */
UCLASS(ClassGroup = (Nova), meta = (BlueprintSpawnableComponent))
class UNovaSpacecraftProcessingSystem
	: public USceneComponent
	, public INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:

	UNovaSpacecraftProcessingSystem();

	/*----------------------------------------------------
	    System implementation
	----------------------------------------------------*/

	virtual void Load(const FNovaSpacecraft& Spacecraft) override;

	virtual void Save(FNovaSpacecraft& Spacecraft) override;

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Get the number of module groups that can support resource processing */
	int32 GetProcessingGroupCount() const
	{
		return ProcessingGroupsStates.Num();
	}

	/** Get input resources for a processing group */
	TArray<const class UNovaResource*> GetInputResources(int32 GroupIndex) const
	{
		NCHECK(GroupIndex >= 0 && GroupIndex < ProcessingGroupsStates.Num());

		TArray<const class UNovaResource*> Result;
		for (const FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[GroupIndex].Chains)
		{
			Result.Append(ChainState.Inputs);
		}

		return Result;
	}

	/** Get output resources for a processing group */
	TArray<const class UNovaResource*> GetOutputResources(int32 GroupIndex) const
	{
		NCHECK(GroupIndex >= 0 && GroupIndex < ProcessingGroupsStates.Num());

		TArray<const class UNovaResource*> Result;
		for (const FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[GroupIndex].Chains)
		{
			Result.Append(ChainState.Outputs);
		}

		return Result;
	}

	/** Get the current processing status for a processing group */
	/*ENovaSpacecraftProcessingSystemStatus GetProcessingGroupStatus(int32 GroupIndex) const
	{
	    NCHECK(GroupIndex >= 0 && GroupIndex < ProcessingGroupsStates.Num());
	    return ProcessingGroupsStates[GroupIndex].Status;
	}*/

	/** Set the active state for a processing group */
	void SetProcessingGroupActive(int32 GroupIndex, bool Active);

	/** Get the real-time cargo state for a specific module slot */
	const FNovaSpacecraftCargo& GetCargo(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		if (IsSpacecraftDocked())
		{
			const FNovaCompartment& Compartment = GetSpacecraft()->Compartments[CompartmentIndex];
			return Compartment.GetCargo(ModuleIndex);
		}
		else
		{
			return RealtimeCompartments[CompartmentIndex].Cargo[ModuleIndex];
		}
	}

	/*----------------------------------------------------
	    Internal
	----------------------------------------------------*/
protected:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Current real-time cargo
	UPROPERTY(Replicated)
	TArray<FNovaSpacecraftProcessingSystemCompartmentCargo> RealtimeCompartments;

	// Processing state
	UPROPERTY(Replicated)
	TArray<FNovaSpacecraftProcessingSystemGroupState> ProcessingGroupsStates;
};
