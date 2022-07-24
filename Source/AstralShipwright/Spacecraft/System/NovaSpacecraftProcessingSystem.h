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

/** Module entry for a chain state */
USTRUCT()
struct FNovaSpacecraftProcessingSystemChainStateModule
{
	GENERATED_BODY();

	UPROPERTY()
	const class UNovaProcessingModuleDescription* Module;

	UPROPERTY()
	int32 CompartmentIndex;

	UPROPERTY()
	int32 ModuleIndex;
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
	TArray<FNovaSpacecraftProcessingSystemChainStateModule> Modules;
};

/** Processing state for a full module group */
USTRUCT()
struct FNovaSpacecraftProcessingSystemGroupState
{
	GENERATED_BODY();

	FNovaSpacecraftProcessingSystemGroupState() : GroupIndex(-1), Active(false)
	{}

	FNovaSpacecraftProcessingSystemGroupState(int32 Index) : GroupIndex(Index), Active(false)
	{}

	UPROPERTY()
	int32 GroupIndex;

	UPROPERTY()
	bool Active;

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

	virtual void Load(const FNovaSpacecraft& Spacecraft) override
	{
		NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
		LoadInternal(Spacecraft);
	}

	virtual void Save(FNovaSpacecraft& Spacecraft) override;

	virtual void Update(FNovaTime InitialTime, FNovaTime FinalTime) override;

	/** Pre-load this system with data so that we can observe it while docked */
	void PreLoad(const FNovaSpacecraft& Spacecraft)
	{
		LoadInternal(Spacecraft);
	}

	/** Get the number of module groups that can support resource processing */
	int32 GetProcessingGroupCount() const
	{
		return ProcessingGroupsStates.Num();
	}

	/** Get the processing group index for a given module */
	int32 GetProcessingGroupIndex(int32 CompartmentIndex, int32 ModuleIndex) const
	{
		int32 CurrentProcessingGroupIndex = 0;
		for (const auto& GroupState : ProcessingGroupsStates)
		{
			for (const FNovaSpacecraftProcessingSystemChainState& ChainState : GroupState.Chains)
			{
				for (const FNovaSpacecraftProcessingSystemChainStateModule& ModuleEntry : ChainState.Modules)
				{
					if (ModuleEntry.CompartmentIndex == CompartmentIndex && ModuleEntry.ModuleIndex == ModuleIndex)
					{
						return CurrentProcessingGroupIndex;
					}
				}
			}
			CurrentProcessingGroupIndex++;
		}

		return INDEX_NONE;
	}

	/** Get the module group for a specific processing group */
	const FNovaModuleGroup& GetModuleGroup(int32 ProcessingGroupIndex) const
	{
		NCHECK(ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num());
		return GetSpacecraft()->GetModuleGroups()[ProcessingGroupsStates[ProcessingGroupIndex].GroupIndex];
	}

	/** Get input resources for a processing group */
	TArray<const class UNovaResource*> GetInputResources(int32 ProcessingGroupIndex) const
	{
		TArray<const class UNovaResource*> Result;

		if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
		{
			for (const FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[ProcessingGroupIndex].Chains)
			{
				Result.Append(ChainState.Inputs);
			}
		}

		return Result;
	}

	/** Get output resources for a processing group */
	TArray<const class UNovaResource*> GetOutputResources(int32 ProcessingGroupIndex) const
	{
		TArray<const class UNovaResource*> Result;

		if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
		{
			for (const FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[ProcessingGroupIndex].Chains)
			{
				Result.Append(ChainState.Outputs);
			}
		}

		return Result;
	}

	/** Get the current processing status for each chain of a processing group */
	TArray<ENovaSpacecraftProcessingSystemStatus> GetProcessingGroupStatus(int32 ProcessingGroupIndex) const;

	/** Get the processing status for a given module */
	ENovaSpacecraftProcessingSystemStatus GetModuleStatus(int32 CompartmentIndex, int32 ModuleIndex) const;

	/** Get all processing chain states for a group */
	TArray<FNovaSpacecraftProcessingSystemChainState> GetChainStates(int32 ProcessingGroupIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetProcessingGroupActive(int32 ProcessingGroupIndex, bool Active);

	/** Set the active state for a processing group */
	void SetProcessingGroupActive(int32 GroupIndex, bool Active)
	{
		if (GetOwner()->GetLocalRole() == ROLE_Authority)
		{
			NCHECK(GroupIndex >= 0 && GroupIndex < ProcessingGroupsStates.Num());
			ProcessingGroupsStates[GroupIndex].Active = Active;
		}
		else
		{
			ServerSetProcessingGroupActive(GroupIndex, Active);
		}
	}

	/** Check the active state for a processing group */
	bool IsProcessingGroupActive(int32 ProcessingGroupIndex) const
	{
		if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
		{
			return ProcessingGroupsStates[ProcessingGroupIndex].Active;
		}

		return false;
	}

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

	/** Processing statuts text getter */
	static FText GetStatusText(ENovaSpacecraftProcessingSystemStatus Type);

	/*----------------------------------------------------
	    Internal
	----------------------------------------------------*/

protected:

	/** Pre-load this system with data so that we can observe it while docked */
	void LoadInternal(const FNovaSpacecraft& Spacecraft);

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
