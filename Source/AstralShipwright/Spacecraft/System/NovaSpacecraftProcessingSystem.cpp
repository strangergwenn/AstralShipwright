// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftProcessingSystem.h"

#include "Spacecraft/NovaSpacecraftTypes.h"

#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "UNovaSpacecraftProcessingSystem"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftProcessingSystem::UNovaSpacecraftProcessingSystem() : Super()
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftProcessingSystem::LoadInternal(const FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftProcessingSystem::LoadInternal");

	MiningRigActive = false;

	// Initialize processing groups with all unique descriptions
	ProcessingGroupsStates.Empty();
	for (const FNovaModuleGroup& Group : Spacecraft.GetModuleGroups())
	{
		// Find out potential mining rig
		const UNovaMiningEquipmentDescription* MiningRig = nullptr;
		for (const FNovaModuleGroupCompartment& GroupCompartment : Group.Compartments)
		{
			const FNovaCompartment& Compartment = Spacecraft.Compartments[GroupCompartment.CompartmentIndex];
			for (FName EquipmentSlotName : GroupCompartment.LinkedEquipments)
			{
				const UNovaMiningEquipmentDescription* MiningRigCandidate =
					Cast<UNovaMiningEquipmentDescription>(Compartment.GetEquipmentySocket(EquipmentSlotName));
				if (MiningRigCandidate)
				{
					MiningRig = MiningRigCandidate;
					break;
				}
			}
		}

		// Find all processing modules
		TArray<FNovaSpacecraftProcessingSystemChainStateModule> ProcessingModules;
		for (const auto& CompartmentModuleIndex : Spacecraft.GetAllModules<UNovaProcessingModuleDescription>(Group))
		{
			const FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentModuleIndex.Key];

			FNovaSpacecraftProcessingSystemChainStateModule Entry;
			Entry.Module           = Cast<UNovaProcessingModuleDescription>(Compartment.Modules[CompartmentModuleIndex.Value].Description);
			Entry.CompartmentIndex = CompartmentModuleIndex.Key;
			Entry.ModuleIndex      = CompartmentModuleIndex.Value;

			ProcessingModules.Add(Entry);
		}

		// Merge modules into chains
		TArray<FNovaSpacecraftProcessingSystemChainState> ProcessingChains;
		for (auto It = ProcessingModules.CreateIterator(); It; ++It)
		{
			const UNovaProcessingModuleDescription* Module = It->Module;

			// Find out whether an already merged chain has valid inputs that this modules provides as output, or vice versa
			FNovaSpacecraftProcessingSystemChainState* ChainState = ProcessingChains.FindByPredicate(
				[Module](const FNovaSpacecraftProcessingSystemChainState& Chain)
				{
					for (const UNovaResource* Resource : Module->Inputs)
					{
						if (Chain.Outputs.Contains(Resource))
						{
							return true;
						}
					}

					for (const UNovaResource* Resource : Module->Outputs)
					{
						if (Chain.Inputs.Contains(Resource))
						{
							return true;
						}
					}

					return false;
				});

			// Update the chain with the current module
			if (ChainState)
			{
				ChainState->Modules.Add(*It);

				// For each new output, either consume an input, or add it
				for (const UNovaResource* Resource : Module->Outputs)
				{
					if (ChainState->Inputs.Contains(Resource))
					{
						ChainState->Inputs.RemoveSingle(Resource);
					}
					else
					{
						ChainState->Outputs.Add(Resource);
					}
				}

				// For each new input, either consume an output, or add it
				for (const UNovaResource* Resource : Module->Inputs)
				{
					if (ChainState->Outputs.Contains(Resource))
					{
						ChainState->Outputs.RemoveSingle(Resource);
					}
					else
					{
						ChainState->Inputs.Add(Resource);
					}
				}
			}

			// Else, add the module as a new chain
			else
			{
				FNovaSpacecraftProcessingSystemChainState NewChainState;

				NewChainState.Inputs         = Module->Inputs;
				NewChainState.Outputs        = Module->Outputs;
				NewChainState.ProcessingRate = Module->ProcessingRate;
				NewChainState.Modules.Add(*It);

				ProcessingChains.Add(NewChainState);
			}

			It.RemoveCurrent();
		}

		// Initialize the processing group and compute the processing rate
		if (ProcessingChains.Num())
		{
			FNovaSpacecraftProcessingSystemGroupState GroupState(Group.Index);
			for (FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingChains)
			{
				if (ChainState.Modules.Num())
				{
					NCHECK(ChainState.Inputs.Num());
					NCHECK(ChainState.Outputs.Num());

					ChainState.ProcessingRate = ChainState.Modules[0].Module->ProcessingRate;
					for (const FNovaSpacecraftProcessingSystemChainStateModule& ModuleEntry : ChainState.Modules)
					{
						ChainState.ProcessingRate = FMath::Min(ChainState.ProcessingRate, ModuleEntry.Module->ProcessingRate);
					}

					GroupState.Chains.Add(ChainState);
				}
			}

			GroupState.MiningRig = MiningRig;

			ProcessingGroupsStates.Add(GroupState);
		}
	}

	// Load all cargo from the spacecraft
	RealtimeCompartments.SetNum(Spacecraft.Compartments.Num());
	for (int32 CompartmentIndex = 0; CompartmentIndex < RealtimeCompartments.Num(); CompartmentIndex++)
	{
		const FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentIndex];
		RealtimeCompartments[CompartmentIndex].Cargo.SetNum(ENovaConstants::MaxModuleCount);

		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			const FNovaSpacecraftCargo& Cargo                         = Compartment.GetCargo(ModuleIndex);
			RealtimeCompartments[CompartmentIndex].Cargo[ModuleIndex] = Cargo;
		}
	}
}

void UNovaSpacecraftProcessingSystem::Save(FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftProcessingSystem::Save");

	// Save all cargo to the spacecraft
	for (int32 CompartmentIndex = 0; CompartmentIndex < RealtimeCompartments.Num(); CompartmentIndex++)
	{
		FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentIndex];

		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(ModuleIndex);

			Cargo = RealtimeCompartments[CompartmentIndex].Cargo[ModuleIndex];
		}
	}

	// Shutdown production
	for (auto& GroupState : ProcessingGroupsStates)
	{
		GroupState.Active = false;
	}
	MiningRigActive = false;
}

void UNovaSpacecraftProcessingSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const FNovaSpacecraft* Spacecraft = GetSpacecraft();

	// Update processing groups
	int32 CurrentGroupIndex = 0;
	for (auto& GroupState : ProcessingGroupsStates)
	{
		for (auto& ChainState : GroupState.Chains)
		{
			// Get required constants for each processing module entry
			const FNovaModuleGroup&            Group             = Spacecraft->GetModuleGroups()[GroupState.GroupIndex];
			const TArray<TPair<int32, int32>>& GroupCargoModules = Spacecraft->GetAllModules<UNovaCargoModuleDescription>(Group);

			// Define processing targets
			float                         MinimumProcessingLeft = FLT_MAX;
			TArray<FNovaSpacecraftCargo*> CurrentInputs;
			TArray<FNovaSpacecraftCargo*> CurrentOutputs;

			// Process cargo for targets
			for (const auto& Indices : GroupCargoModules)
			{
				FNovaSpacecraftCargo& Cargo         = RealtimeCompartments[Indices.Key].Cargo[Indices.Value];
				const float           CargoCapacity = Spacecraft->GetCargoCapacity(Indices.Key, Indices.Value);

				// Valid resource input
				if (Cargo.Resource && ChainState.Inputs.Contains(Cargo.Resource) && Cargo.Amount > 0)
				{
					MinimumProcessingLeft = FMath::Min(MinimumProcessingLeft, Cargo.Amount);
					CurrentInputs.AddUnique(&Cargo);
				}

				// Valid resource output
				else if (Cargo.Resource && ChainState.Outputs.Contains(Cargo.Resource) && Cargo.Amount < CargoCapacity)
				{
					MinimumProcessingLeft = FMath::Min(MinimumProcessingLeft, CargoCapacity - Cargo.Amount);
					CurrentOutputs.AddUnique(&Cargo);
				}
			}

			if (IsSpacecraftDocked())
			{
				ChainState.Status = ENovaSpacecraftProcessingSystemStatus::Docked;
			}
			else
			{
				// Start production based on input
				if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Stopped ||
					ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
				{
					ChainState.Status = GroupState.Active ? ENovaSpacecraftProcessingSystemStatus::Processing
					                                      : ENovaSpacecraftProcessingSystemStatus::Stopped;
				}

				// Cancel production when blocked
				if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing &&
					(CurrentInputs.Num() != ChainState.Inputs.Num() || CurrentOutputs.Num() != ChainState.Outputs.Num()))
				{
					ChainState.Status = ENovaSpacecraftProcessingSystemStatus::Blocked;
					NLOG("UNovaSpacecraftProcessingSystem::Update : canceled production for group %d", CurrentGroupIndex);
				}

				// Proceed with processing
				if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
				{
					float ResourceDelta = ChainState.ProcessingRate * (FinalTime - InitialTime).AsSeconds();
					ResourceDelta       = FMath::Min(ResourceDelta, MinimumProcessingLeft);
					NCHECK(ResourceDelta > 0);

					for (FNovaSpacecraftCargo* Input : CurrentInputs)
					{
						Input->Amount -= ResourceDelta;
					}

					for (FNovaSpacecraftCargo* Output : CurrentOutputs)
					{
						Output->Amount += ResourceDelta;
					}
				}
			}
		}

		CurrentGroupIndex++;
	}

	// Process the mining rig
	if (IsSpacecraftDocked())
	{
		MiningRigStatus = ENovaSpacecraftProcessingSystemStatus::Docked;
	}
	else
	{
		// TODO
		MiningRigStatus =
			MiningRigActive ? ENovaSpacecraftProcessingSystemStatus::Processing : ENovaSpacecraftProcessingSystemStatus::Stopped;
	}
}

TArray<ENovaSpacecraftProcessingSystemStatus> UNovaSpacecraftProcessingSystem::GetProcessingGroupStatus(int32 ProcessingGroupIndex) const
{
	TArray<ENovaSpacecraftProcessingSystemStatus> Result;

	if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
	{
		for (const FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[ProcessingGroupIndex].Chains)
		{
			Result.Add(ChainState.Status);
		}
	}

	return Result;
}

ENovaSpacecraftProcessingSystemStatus UNovaSpacecraftProcessingSystem::GetModuleStatus(int32 CompartmentIndex, int32 ModuleIndex) const
{
	for (const auto& GroupState : ProcessingGroupsStates)
	{
		for (const FNovaSpacecraftProcessingSystemChainState& ChainState : GroupState.Chains)
		{
			for (const FNovaSpacecraftProcessingSystemChainStateModule& ModuleEntry : ChainState.Modules)
			{
				if (ModuleEntry.CompartmentIndex == CompartmentIndex && ModuleEntry.ModuleIndex == ModuleIndex)
				{
					return ChainState.Status;
				}
			}
		}
	}

	return ENovaSpacecraftProcessingSystemStatus::Docked;
}

TArray<FNovaSpacecraftProcessingSystemChainState> UNovaSpacecraftProcessingSystem::GetChainStates(int32 ProcessingGroupIndex)
{
	if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
	{
		return ProcessingGroupsStates[ProcessingGroupIndex].Chains;
	}

	return TArray<FNovaSpacecraftProcessingSystemChainState>();
}

FText UNovaSpacecraftProcessingSystem::GetStatusText(ENovaSpacecraftProcessingSystemStatus Type)
{
	switch (Type)
	{
		default:
		case ENovaSpacecraftProcessingSystemStatus::Stopped:
			return LOCTEXT("ProcessingStopped", "Stopped");
		case ENovaSpacecraftProcessingSystemStatus::Processing:
			return LOCTEXT("ProcessingProcessing", "Active");
		case ENovaSpacecraftProcessingSystemStatus::Blocked:
			return LOCTEXT("ProcessingBlocked", "Blocked");
		case ENovaSpacecraftProcessingSystemStatus::Docked:
			return LOCTEXT("ProcessingDocked", "Stopped");
	}
}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

bool UNovaSpacecraftProcessingSystem::ServerSetProcessingGroupActive_Validate(int32 ProcessingGroupIndex, bool Active)
{
	return true;
}

void UNovaSpacecraftProcessingSystem::ServerSetProcessingGroupActive_Implementation(int32 ProcessingGroupIndex, bool Active)
{
	SetProcessingGroupActive(ProcessingGroupIndex, Active);
}

bool UNovaSpacecraftProcessingSystem::ServerSetMiningRigActive_Validate(bool Active)
{
	return true;
}

void UNovaSpacecraftProcessingSystem::ServerSetMiningRigActive_Implementation(bool Active)
{
	SetMiningRigActive(Active);
}

void UNovaSpacecraftProcessingSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, RealtimeCompartments);
	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, ProcessingGroupsStates);
	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, MiningRigActive);
	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, MiningRigStatus);
}

#undef LOCTEXT_NAMESPACE
