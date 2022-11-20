// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftProcessingSystem.h"
#include "NovaSpacecraftPowerSystem.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Spacecraft/NovaSpacecraftTypes.h"

#include "Game/NovaAsteroid.h"

#include "Neutron/System/NeutronAssetManager.h"

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

void UNovaSpacecraftProcessingSystem::Load(const FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftProcessingSystem::Load");

	MiningRigActive   = false;
	MiningRigResource = nullptr;
	MiningRigStatus   = ENovaSpacecraftProcessingSystemStatus::Stopped;

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
		if (ProcessingChains.Num() || MiningRig)
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

	MiningRigActive   = false;
	MiningRigResource = nullptr;
}

void UNovaSpacecraftProcessingSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const FNovaSpacecraft*            Spacecraft  = GetSpacecraft();
	const ANovaGameState*             GameState   = GetWorld()->GetGameState<ANovaGameState>();
	const UNovaSpacecraftPowerSystem* PowerSystem = GameState->GetSpacecraftSystem<UNovaSpacecraftPowerSystem>(Spacecraft);

	RemainingProductionTime = FNovaTime::FromMinutes(DBL_MAX);

	// Update processing groups
	int32 CurrentGroupIndex = 0;
	for (auto& GroupState : ProcessingGroupsStates)
	{
		for (auto& ChainState : GroupState.Chains)
		{
			if (IsSpacecraftDocked())
			{
				ChainState.Status = ENovaSpacecraftProcessingSystemStatus::Docked;
			}
			else
			{
				// Get required constants for each processing module entry
				const FNovaModuleGroup&            Group             = Spacecraft->GetModuleGroups()[GroupState.GroupIndex];
				const TArray<TPair<int32, int32>>& GroupCargoModules = Spacecraft->GetAllModules<UNovaCargoModuleDescription>(Group);

				// Define processing targets
				int32                              EmptyOutputSlotCount = 0;
				TArray<FNovaSpacecraftCargo*>      InputCargoSlots;
				TArray<FNovaSpacecraftCargo*>      OutputCargoSlots;
				TMap<FNovaSpacecraftCargo*, float> CargoSlotCapacities;
				TMap<const UNovaResource*, float>  InputResourceAmounts;
				TMap<const UNovaResource*, float>  OutputResourceAmounts;

				// Process cargo for targets, identify empty slots
				float MinCargoCapacity = 1000;
				for (const auto& Indices : GroupCargoModules)
				{
					FNovaSpacecraftCargo& Cargo         = RealtimeCompartments[Indices.Key].Cargo[Indices.Value];
					const float           CargoCapacity = Spacecraft->GetCargoCapacity(Indices.Key, Indices.Value);
					MinCargoCapacity                    = FMath::Min(MinCargoCapacity, CargoCapacity);
					CargoSlotCapacities.Add(&Cargo, CargoCapacity);

					// Valid resource input: add to the input map, and the list of current input slots
					if (Cargo.Resource && ChainState.Inputs.Contains(Cargo.Resource) && Cargo.Amount > 0)
					{
						auto& ResourceAmount = InputResourceAmounts.FindOrAdd(Cargo.Resource);
						ResourceAmount += Cargo.Amount;
						InputCargoSlots.AddUnique(&Cargo);
					}

					// Valid resource output: add to the output map, the list of current output slots, process empty modifier
					else if ((ChainState.Outputs.Contains(Cargo.Resource) || Cargo.Resource == nullptr) && Cargo.Amount < CargoCapacity)
					{
						if (Cargo.Resource)
						{
							auto& ResourceAmount = OutputResourceAmounts.FindOrAdd(Cargo.Resource);
							ResourceAmount += CargoCapacity - Cargo.Amount;
						}
						else
						{
							EmptyOutputSlotCount++;
						}

						OutputCargoSlots.AddUnique(&Cargo);
					}
				}

				// Ensure all output resources have at least an empty entry
				for (const UNovaResource* Resource : ChainState.Outputs)
				{
					if (!OutputResourceAmounts.Contains(Resource))
					{
						OutputResourceAmounts.Add(Resource, 0);
					}
				}

				// Attribute empty slots to the resources with the least space
				OutputResourceAmounts.ValueSort(TLess<float>());
				int32 RemainingEmptySlots = EmptyOutputSlotCount;
				for (TPair<const UNovaResource*, float>& Output : OutputResourceAmounts)
				{
					if (RemainingEmptySlots > 0)
					{
						Output.Value += MinCargoCapacity;
						RemainingEmptySlots--;
					}
				}

				// Start production based on input
				if (!GroupState.Active)
				{
					ChainState.Status = ENovaSpacecraftProcessingSystemStatus::Stopped;
				}
				else if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Stopped)
				{
					ChainState.Status = ENovaSpacecraftProcessingSystemStatus::Processing;
				}

				// Cancel production when blocked
				if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
				{
					auto CargoHasResource = [](const TArray<FNovaSpacecraftCargo*>& Entries, const UNovaResource* Resource)
					{
						for (const FNovaSpacecraftCargo* Cargo : Entries)
						{
							if (Cargo->Resource == Resource)
							{
								return true;
							}
						}
						return false;
					};

					bool HasMissingResource = false;

					// Check inputs
					for (const UNovaResource* Resource : ChainState.Inputs)
					{
						if (!CargoHasResource(InputCargoSlots, Resource))
						{
							HasMissingResource = true;
							break;
						}
					}

					// Check outputs, allow empty cargo
					for (const UNovaResource* Resource : ChainState.Outputs)
					{
						if (CargoHasResource(OutputCargoSlots, Resource))
						{}
						else if (EmptyOutputSlotCount > 0 && CargoHasResource(OutputCargoSlots, nullptr))
						{
							EmptyOutputSlotCount--;
						}
						else
						{
							HasMissingResource = true;
							break;
						}
					}

					if (HasMissingResource)
					{
						ChainState.Status = ENovaSpacecraftProcessingSystemStatus::Blocked;
						NLOG("UNovaSpacecraftProcessingSystem::Update : blocked production for group %d", CurrentGroupIndex);
					}
				}

				// Cancel production when out of power
				if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing && PowerSystem->GetRemainingEnergy() <= 0)
				{
					ChainState.Status = ENovaSpacecraftProcessingSystemStatus::PowerLoss;
					NLOG("UNovaSpacecraftProcessingSystem::Update : power loss for group %d", CurrentGroupIndex);
				}

				// Proceed with processing
				if (ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
				{
					float     MinimumProcessingLeft   = FLT_MAX;
					FNovaTime TotalChainTimeRemaining = FLT_MAX;

					auto UpdateTotalTimeRemaining = [&MinimumProcessingLeft, &TotalChainTimeRemaining, &ChainState](
														const TMap<const UNovaResource*, float>&  ResourceAmounts,
														const TArray<const class UNovaResource*>& ProductionResources)
					{
						for (const TPair<const UNovaResource*, float>& Input : ResourceAmounts)
						{
							// Compute the processing rate
							int32 InputProcessorCount = 0;
							for (const UNovaResource* Resource : ProductionResources)
							{
								if (Resource == Input.Key)
								{
									InputProcessorCount++;
								}
							}
							const float ProcessingRate = InputProcessorCount * ChainState.ProcessingRate;
							TotalChainTimeRemaining =
								FMath::Min(TotalChainTimeRemaining, FNovaTime::FromSeconds(Input.Value / ProcessingRate));
							MinimumProcessingLeft = FMath::Min(MinimumProcessingLeft, Input.Value);
						}
					};

					// Process the total processing time remaining
					UpdateTotalTimeRemaining(InputResourceAmounts, ChainState.Inputs);
					UpdateTotalTimeRemaining(OutputResourceAmounts, ChainState.Outputs);
					RemainingProductionTime = FMath::Min(RemainingProductionTime, TotalChainTimeRemaining);

					// NLOG("%.2f minutes remaining, min %.2f, processing %.2f", TotalChainTimeRemaining.AsMinutes(), MinimumProcessingLeft,
					//	(FinalTime - InitialTime).AsMinutes());

					const float ResourceDelta =
						FMath::Min(ChainState.ProcessingRate * (FinalTime - InitialTime).AsSeconds(), MinimumProcessingLeft);
					if (ResourceDelta > 0)
					{
						// Process outputs, prefer existing resource
						for (const UNovaResource* Resource : ChainState.Outputs)
						{
							float CurrentDelta = ResourceDelta;

							auto ProcessOutput = [Resource, &CurrentDelta, &CargoSlotCapacities](
													 FNovaSpacecraftCargo* Cargo, bool AcceptEmpty)
							{
								float LocalResourceDelta = FMath::Min(CurrentDelta, CargoSlotCapacities[Cargo] - Cargo->Amount);
								if (LocalResourceDelta > 0 && (Cargo->Resource == Resource || Cargo->Resource == nullptr && AcceptEmpty))
								{
									Cargo->Resource = Resource;
									Cargo->Amount += LocalResourceDelta;

									CurrentDelta = FMath::Max(CurrentDelta - LocalResourceDelta, 0);
								}
							};

							for (FNovaSpacecraftCargo* Output : OutputCargoSlots)
							{
								ProcessOutput(Output, false);
							}

							if (ResourceDelta > 0)
							{
								for (FNovaSpacecraftCargo* Output : OutputCargoSlots)
								{
									ProcessOutput(Output, true);
								}
							}

							NCHECK(CurrentDelta == 0);
						}

						// Process inputs
						for (const UNovaResource* Resource : ChainState.Inputs)
						{
							float CurrentDelta = ResourceDelta;

							for (FNovaSpacecraftCargo* Input : InputCargoSlots)
							{
								if (Input->Resource == Resource)
								{
									float LocalResourceDelta = FMath::Min(CurrentDelta, Input->Amount);

									Input->Amount -= LocalResourceDelta;
									if (Input->Amount < ENovaConstants::ResourceQuantityThreshold)
									{
										Input->Resource = nullptr;
										Input->Amount   = 0;
									}

									CurrentDelta -= LocalResourceDelta;
								}
							}

							NCHECK(CurrentDelta == 0);
						}
					}
				}
			}
		}

		CurrentGroupIndex++;
	}

	// Process the mining rig
	MiningRigResource = nullptr;
	if (IsSpacecraftDocked())
	{
		MiningRigStatus = ENovaSpacecraftProcessingSystemStatus::Docked;
	}
	else
	{
		// Find spacecraft, asteroid
		ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
		NCHECK(SpacecraftPawn);
		const ANovaAsteroid* AsteroidActor =
			UNeutronActorTools::GetClosestActor<ANovaAsteroid>(SpacecraftPawn, SpacecraftPawn->GetActorLocation());

		// Process activity
		if (MiningRigActive && IsValid(AsteroidActor) && CanMiningRigBeActive())
		{
			// Get data
			const FNovaAsteroid&               Asteroid          = Cast<ANovaAsteroid>(AsteroidActor)->GetAsteroidData();
			const int32                        GroupIndex        = GetMiningRigIndex();
			const FNovaModuleGroup&            Group             = Spacecraft->GetModuleGroups()[GroupIndex];
			const TArray<TPair<int32, int32>>& GroupCargoModules = Spacecraft->GetAllModules<UNovaCargoModuleDescription>(Group);
			TMap<FNovaSpacecraftCargo*, float> CargoSlotCapacities;

			// Define processing targets
			MiningRigResource                                 = Asteroid.MineralResource;
			float                         TotalProcessingLeft = 0;
			TArray<FNovaSpacecraftCargo*> CurrentOutputs;

			// Process cargo for targets
			for (const auto& Indices : GroupCargoModules)
			{
				FNovaSpacecraftCargo& Cargo         = RealtimeCompartments[Indices.Key].Cargo[Indices.Value];
				const float           CargoCapacity = Spacecraft->GetCargoCapacity(Indices.Key, Indices.Value);
				CargoSlotCapacities.Add(&Cargo, CargoCapacity);

				// Valid resource output
				if ((MiningRigResource == Cargo.Resource || Cargo.Resource == nullptr) && Cargo.Amount < CargoCapacity)
				{
					TotalProcessingLeft += CargoCapacity - Cargo.Amount;
					CurrentOutputs.AddUnique(&Cargo);
				}
			}

			// Cancel production when blocked
			if (CurrentOutputs.Num() == 0)
			{
				MiningRigStatus = ENovaSpacecraftProcessingSystemStatus::Blocked;
				NLOG("UNovaSpacecraftProcessingSystem::Update : canceled production for mining rig");
			}

			// Cancel production when out of power
			else if (PowerSystem->GetRemainingEnergy() <= 0)
			{
				MiningRigStatus = ENovaSpacecraftProcessingSystemStatus::PowerLoss;
				NLOG("UNovaSpacecraftProcessingSystem::Update : power loss for mining rig");
			}

			// Proceed with processing
			else
			{
				// Compute the mining delta
				MiningRigStatus     = ENovaSpacecraftProcessingSystemStatus::Processing;
				float ResourceDelta = GetCurrentMiningRate() * (FinalTime - InitialTime).AsSeconds();
				ResourceDelta       = FMath::Min(ResourceDelta, TotalProcessingLeft);
				if (ResourceDelta > 0)
				{
					auto ProcessOutput = [this, &ResourceDelta, &CargoSlotCapacities](FNovaSpacecraftCargo* Cargo, bool AcceptEmpty)
					{
						float LocalResourceDelta = FMath::Min(ResourceDelta, CargoSlotCapacities[Cargo] - Cargo->Amount);
						if (LocalResourceDelta > 0 && (Cargo->Resource == MiningRigResource || Cargo->Resource == nullptr && AcceptEmpty))
						{
							Cargo->Resource = MiningRigResource;
							Cargo->Amount += LocalResourceDelta;

							ResourceDelta = FMath::Max(ResourceDelta - LocalResourceDelta, 0);
						}
					};

					for (FNovaSpacecraftCargo* Output : CurrentOutputs)
					{
						ProcessOutput(Output, false);
					}

					if (ResourceDelta > 0)
					{
						for (FNovaSpacecraftCargo* Output : CurrentOutputs)
						{
							ProcessOutput(Output, true);
						}
					}

					NCHECK(ResourceDelta == 0);

					// Compute remaining time for simulation purposes
					const FNovaTime TotalMiningTimeRemaining = FNovaTime::FromSeconds(TotalProcessingLeft / GetCurrentMiningRate());
					RemainingProductionTime                  = FMath::Min(RemainingProductionTime, TotalMiningTimeRemaining);

					NLOG("%.2f minutes remaining, min %.2f, processing %.2f", TotalMiningTimeRemaining.AsMinutes(), TotalProcessingLeft,
						(FinalTime - InitialTime).AsMinutes());
				}
			}
		}
		else if (MiningRigStatus != ENovaSpacecraftProcessingSystemStatus::Blocked &&
				 MiningRigStatus != ENovaSpacecraftProcessingSystemStatus::PowerLoss)
		{
			MiningRigStatus = ENovaSpacecraftProcessingSystemStatus::Stopped;
		}
	}

	// Account for the time we're literally in the process of simulating
	if (RemainingProductionTime < FLT_MAX)
	{
		RemainingProductionTime -= (FinalTime - InitialTime);
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

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

TArray<FNovaSpacecraftProcessingSystemChainState> UNovaSpacecraftProcessingSystem::GetChainStates(int32 ProcessingGroupIndex)
{
	if (ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num())
	{
		return ProcessingGroupsStates[ProcessingGroupIndex].Chains;
	}

	return TArray<FNovaSpacecraftProcessingSystemChainState>();
}

float UNovaSpacecraftProcessingSystem::GetCurrentMiningRate() const
{
	ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
	NCHECK(SpacecraftPawn);
	const ANovaAsteroid* AsteroidActor =
		UNeutronActorTools::GetClosestActor<ANovaAsteroid>(SpacecraftPawn, SpacecraftPawn->GetActorLocation());

	if (IsValid(AsteroidActor))
	{
		for (const auto& GroupState : ProcessingGroupsStates)
		{
			if (GroupState.MiningRig)
			{
				const FVector SpacecraftRelativeLocation =
					AsteroidActor->GetTransform().InverseTransformPosition(SpacecraftPawn->GetActorLocation());

				return GroupState.MiningRig->ExtractionRate * AsteroidActor->GetMineralDensity(SpacecraftRelativeLocation);
			}
		}
	}

	return 0.0f;
}

bool UNovaSpacecraftProcessingSystem::CanMiningRigBeActive(FText* Help) const
{
	if (GetMiningRigIndex() == INDEX_NONE)
	{
		if (Help)
		{
			*Help = LOCTEXT("NotRig", "This spacecraft doesn't have a mining rig");
		}
		return false;
	}
	else if (GetMiningRigStatus() == ENovaSpacecraftProcessingSystemStatus::Blocked)
	{
		if (Help)
		{
			*Help = LOCTEXT("Blocked", "The mining rig cannot store extracted resources");
		}
		return false;
	}
	else if (GetMiningRigStatus() == ENovaSpacecraftProcessingSystemStatus::PowerLoss)
	{
		if (Help)
		{
			*Help = LOCTEXT("PowerLoss", "The mining rig is out of power");
		}
		return false;
	}
	else
	{
		ANovaSpacecraftPawn* SpacecraftPawn = GetOwner<ANovaSpacecraftPawn>();
		NCHECK(SpacecraftPawn);

		if (!IsValid(SpacecraftPawn) || !SpacecraftPawn->GetSpacecraftMovement()->IsAnchored())
		{
			if (Help)
			{
				*Help = LOCTEXT("NotAnchored", "The mining rig can only be engaged while anchored to an asteroid");
			}
			return false;
		}
		else
		{
			return true;
		}
	}
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
		case ENovaSpacecraftProcessingSystemStatus::PowerLoss:
			return LOCTEXT("ProcessingPowerLoss", "No power");
		case ENovaSpacecraftProcessingSystemStatus::Docked:
			return LOCTEXT("ProcessingDocked", "Stopped");
	}
}

int32 UNovaSpacecraftProcessingSystem::GetProcessingGroupCrew(int32 ProcessingGroupIndex, bool FilterByActive) const
{
	NCHECK(ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num());

	int32 Count = 0;

	for (const auto& ChainState : ProcessingGroupsStates[ProcessingGroupIndex].Chains)
	{
		if (FilterByActive == false || ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
		{
			for (const auto& ModuleState : ChainState.Modules)
			{
				if (ModuleState.Module->CrewEffect < 0)
				{
					Count += FMath::Abs(ModuleState.Module->CrewEffect);
				}
			}
		}
	}

	return Count;
}

int32 UNovaSpacecraftProcessingSystem::GetProcessingGroupPowerUsage(int32 ProcessingGroupIndex, bool FilterByActive) const
{
	NCHECK(ProcessingGroupIndex >= 0 && ProcessingGroupIndex < ProcessingGroupsStates.Num());

	int32 Power = 0;

	for (const auto& ChainState : ProcessingGroupsStates[ProcessingGroupIndex].Chains)
	{
		if (FilterByActive == false || ChainState.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
		{
			for (const auto& ModuleState : ChainState.Modules)
			{
				Power += ModuleState.Module->Power;
			}
		}
	}

	return Power;
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
	DOREPLIFETIME(UNovaSpacecraftProcessingSystem, MiningRigResource);
}

#undef LOCTEXT_NAMESPACE
