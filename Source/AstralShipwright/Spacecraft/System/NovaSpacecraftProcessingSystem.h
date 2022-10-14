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
	PowerLoss,
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

	FNovaSpacecraftProcessingSystemChainStateModule() : Module(nullptr), CompartmentIndex(INDEX_NONE), ModuleIndex(INDEX_NONE)
	{}

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

	FNovaSpacecraftProcessingSystemChainState() : Status(ENovaSpacecraftProcessingSystemStatus::Stopped), ProcessingRate(0)
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

	// Replicated module list
	UPROPERTY()
	TArray<FNovaSpacecraftProcessingSystemChainStateModule> Modules;
};

/** Processing state for a full module group */
USTRUCT()
struct FNovaSpacecraftProcessingSystemGroupState
{
	GENERATED_BODY();

	FNovaSpacecraftProcessingSystemGroupState() : GroupIndex(-1), Active(false), MiningRig(nullptr)
	{}

	FNovaSpacecraftProcessingSystemGroupState(int32 Index) : GroupIndex(Index), Active(false), MiningRig(nullptr)
	{}

	// Module group index
	UPROPERTY()
	int32 GroupIndex;

	// Processing group activity
	UPROPERTY()
	bool Active;

	// Processing chains
	UPROPERTY()
	TArray<FNovaSpacecraftProcessingSystemChainState> Chains;

	// Mining rig
	UPROPERTY()
	const class UNovaMiningEquipmentDescription* MiningRig;
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

	/*----------------------------------------------------
	    Processing groups
	----------------------------------------------------*/

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

	/** Get the current processing status for each chain of a processing group */
	TArray<ENovaSpacecraftProcessingSystemStatus> GetProcessingGroupStatus(int32 ProcessingGroupIndex) const;

	/** Get the processing status for a given module */
	ENovaSpacecraftProcessingSystemStatus GetModuleStatus(int32 CompartmentIndex, int32 ModuleIndex) const;

	/** Get the processing status for the mining rig */
	ENovaSpacecraftProcessingSystemStatus GetMiningRigStatus() const
	{
		return MiningRigStatus;
	}

	/** Get the remaining time this system can remain active */
	FNovaTime GetRemainingProductionTime() const
	{
		NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
		return RemainingProductionTime;
	}

	/*----------------------------------------------------
	    Resources
	----------------------------------------------------*/

	/** Get input resources for a processing group */
	TArray<const class UNovaResource*> GetInputResources(int32 ProcessingGroupIndex) const
	{
		TArray<const class UNovaResource*> Result;

		if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
		{
			for (const FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[ProcessingGroupIndex].Chains)
			{
				for (const class UNovaResource* Resource : ChainState.Inputs)
				{
					Result.AddUnique(Resource);
				}
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
				for (const class UNovaResource* Resource : ChainState.Outputs)
				{
					Result.AddUnique(Resource);
				}
			}
		}

		return Result;
	}

	/** Get all processing chain states for a group */
	TArray<FNovaSpacecraftProcessingSystemChainState> GetChainStates(int32 ProcessingGroupIndex);

	/** Get the module group that matches a mining rig */
	int32 GetMiningRigIndex() const
	{
		for (const auto& GroupState : ProcessingGroupsStates)
		{
			if (GroupState.MiningRig)
			{
				return GroupState.GroupIndex;
			}
		}

		return INDEX_NONE;
	}

	/** Get the currently mined resource */
	const class UNovaResource* GetCurrentMiningResource() const
	{
		return MiningRigResource;
	}

	/** GEt the current mining rate in T/S */
	float GetCurrentMiningRate() const;

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
	    Activity control
	----------------------------------------------------*/

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetProcessingGroupActive(int32 ProcessingGroupIndex, bool Active);

	/** Set the active state for a processing group */
	void SetProcessingGroupActive(int32 GroupIndex, bool Active)
	{
		NLOG("UNovaSpacecraftProcessingSystem::SetProcessingGroupActive : %d=%d (%s)", GroupIndex, Active, *GetRoleString(this));

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

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetMiningRigActive(bool Active);

	/** Set the active state for the mining rig */
	void SetMiningRigActive(bool Active)
	{
		NLOG("UNovaSpacecraftProcessingSystem::SetMiningRigActive : %d (%s)", Active, *GetRoleString(this));

		if (GetOwner()->GetLocalRole() == ROLE_Authority)
		{
			MiningRigActive = Active;
		}
		else
		{
			ServerSetMiningRigActive(Active);
		}
	}

	/** Check whether he mining rig can become active */
	bool CanMiningRigBeActive(FText* Help = nullptr) const;

	/** Check the active state for the mining rig */
	bool IsMiningRigActive() const
	{
		return MiningRigActive;
	}

	/** Processing status text getter */
	static FText GetStatusText(ENovaSpacecraftProcessingSystemStatus Type);

	/*----------------------------------------------------
	    Crew
	----------------------------------------------------*/

	/** Get the current busy crew count for a group */
	int32 GetBusyCrew(int32 ProcessingGroupIndex) const
	{
		return GetProcessingGroupCrew(ProcessingGroupIndex, true);
	}

	/** Get the required crew count for a group to be active */
	int32 GetRequiredCrew(int32 ProcessingGroupIndex) const
	{
		return GetProcessingGroupCrew(ProcessingGroupIndex, false);
	}

	/** Get the total current busy crew count */
	int32 GetTotalBusyCrew() const;

	/** Get the total current available crew count */
	int32 GetTotalAvailableCrew() const
	{
		return FMath::Max(GetTotalCrew() - GetTotalBusyCrew(), 0);
	}

	/** Get the total crew count */
	int32 GetTotalCrew() const;

protected:

	/** Get the crew count for a processing group, either total or active */
	int32 GetProcessingGroupCrew(int32 ProcessingGroupIndex, bool FilterByActive) const;

	/*----------------------------------------------------
	    Power
	----------------------------------------------------*/

public:

	/** Get the current power usage for a group */
	int32 GetPowerUsage(int32 ProcessingGroupIndex) const
	{
		return GetProcessingGroupPowerUsage(ProcessingGroupIndex, true);
	}

	/** Get the required power usage for a group to be active */
	int32 GetRequiredPowerUsage(int32 ProcessingGroupIndex) const
	{
		return GetProcessingGroupPowerUsage(ProcessingGroupIndex, false);
	}

protected:

	/** Get the power usage for a processing group, either total or active */
	int32 GetProcessingGroupPowerUsage(int32 ProcessingGroupIndex, bool FilterByActive) const;

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

	// Mining state
	UPROPERTY(Replicated)
	bool MiningRigActive;

	// Mining status
	UPROPERTY(Replicated)
	ENovaSpacecraftProcessingSystemStatus MiningRigStatus;

	// Mining status
	UPROPERTY(Replicated)
	const class UNovaResource* MiningRigResource;

	// Server-side status
	FNovaTime RemainingProductionTime;
};
