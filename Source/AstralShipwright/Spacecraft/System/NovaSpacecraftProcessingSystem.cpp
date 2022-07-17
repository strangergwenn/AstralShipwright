// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftProcessingSystem.h"

#include "Spacecraft/NovaSpacecraftTypes.h"

#include "Net/UnrealNetwork.h"

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

void UNovaSpacecraftProcessingSystem::Load(const FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftProcessingSystem::Load");

	// Initialize processing groups with all unique descriptions
	for (const FNovaModuleGroup& Group : Spacecraft.GetModuleGroups())
	{
		// Find all processing modules
		TArray<const UNovaProcessingModuleDescription*> ProcessingModules;
		for (const auto& CompartmentModuleIndex : GetSpacecraft()->GetAllModules<UNovaProcessingModuleDescription>(Group))
		{
			const FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentModuleIndex.Key];
			ProcessingModules.Add(Cast<UNovaProcessingModuleDescription>(Compartment.Modules[CompartmentModuleIndex.Value].Description));
		}

		// Merge modules into chains
		TArray<FNovaSpacecraftProcessingSystemChainState> MergedProcessingModules;
		for (auto It = ProcessingModules.CreateIterator(); It; ++It)
		{
			const UNovaProcessingModuleDescription* Module = *It;

			// Find out whether an already merged chain has valid inputs that this modules provides as output, or vice versa
			FNovaSpacecraftProcessingSystemChainState* ChainState = MergedProcessingModules.FindByPredicate(
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

			if (ChainState)
			{
				ChainState->Modules.Add(Module);

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

				It.RemoveCurrent();
				continue;
			}
		}

		// Initialize the processing group and compute the processing rate
		FNovaSpacecraftProcessingSystemGroupState GroupState;
		for (FNovaSpacecraftProcessingSystemChainState& ChainState : MergedProcessingModules)
		{
			if (ChainState.Modules.Num())
			{
				NCHECK(ChainState.Inputs.Num());
				NCHECK(ChainState.Outputs.Num());

				ChainState.ProcessingRate = ChainState.Modules[0]->ProcessingRate;
				for (const UNovaProcessingModuleDescription* Module : ChainState.Modules)
				{
					ChainState.ProcessingRate = FMath::Min(ChainState.ProcessingRate, Module->ProcessingRate);
				}

				GroupState.Chains.Add(ChainState);
			}
		}

		ProcessingGroupsStates.Add(GroupState);
	}

	// Load all cargo from the spacecraft
	RealtimeCompartments.SetNumUninitialized(GetSpacecraft()->Compartments.Num());
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

	RealtimeCompartments.Empty();
}

void UNovaSpacecraftProcessingSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const FNovaSpacecraft* Spacecraft        = GetSpacecraft();
	int32                  CurrentGroupIndex = 0;

	for (auto& GroupState : ProcessingGroupsStates)
	{
		for (auto& ChainState : GroupState.Chains)
		{
			// Get required constants for each processing module entry
			const FNovaModuleGroup&            Group             = Spacecraft->GetModuleGroups()[CurrentGroupIndex];
			const TArray<TPair<int32, int32>>& GroupCargoModules = GetSpacecraft()->GetAllModules<UNovaCargoModuleDescription>(Group);
			const TArray<const UNovaProcessingModuleDescription*> GroupProcessingModules = ChainState.Modules;

			// Define processing targets
			float                         MinimumProcessingLeft = FLT_MAX;
			TArray<FNovaSpacecraftCargo*> CurrentInputs;
			TArray<FNovaSpacecraftCargo*> CurrentOutputs;

			// Process cargo for targets
			for (const auto& Indices : GroupCargoModules)
			{
				FNovaSpacecraftCargo& Cargo         = RealtimeCompartments[Indices.Key].Cargo[Indices.Value];
				const float           CargoCapacity = GetSpacecraft()->GetCargoCapacity(Indices.Key, Indices.Value);

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

		CurrentGroupIndex++;
	}
}

void UNovaSpacecraftProcessingSystem::SetProcessingGroupActive(int32 GroupIndex, bool Active)
{
	NCHECK(GroupIndex >= 0 && GroupIndex < ProcessingGroupsStates.Num());

	for (FNovaSpacecraftProcessingSystemChainState& ChainState : ProcessingGroupsStates[GroupIndex].Chains)
	{
		bool StateBlocked = ChainState.Status != ENovaSpacecraftProcessingSystemStatus::Blocked ||
		                    ChainState.Status != ENovaSpacecraftProcessingSystemStatus::Docked;

		ChainState.Status =
			(Active && !StateBlocked) ? ENovaSpacecraftProcessingSystemStatus::Processing : ENovaSpacecraftProcessingSystemStatus::Stopped;
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaSpacecraftProcessingSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, RealtimeCompartments);
	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, ProcessingGroupsStates);
}
