// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftProcessingSystem.h"

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
	NLOG("UNovaSpacecraftProcessingSystem::Load");

	// Load all cargo from the spacecraft
	RealtimeCargo.SetNumUninitialized(GetSpacecraft()->Compartments.Num());
	for (int32 CompartmentIndex = 0; CompartmentIndex < RealtimeCargo.Num(); CompartmentIndex++)
	{
		const FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentIndex];

		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			// Handle processing modules
			const UNovaProcessingModuleDescription* Module =
				Cast<UNovaProcessingModuleDescription>(Compartment.Modules[ModuleIndex].Description);
			if (Module)
			{
				ModuleStates.Add(TPair<int32, int32>(CompartmentIndex, ModuleIndex), FNovaSpacecraftProcessingSystemModuleState(Module));
			}

			// Handle cargo
			const FNovaSpacecraftCargo& Cargo            = Compartment.GetCargo(ModuleIndex);
			RealtimeCargo[CompartmentIndex][ModuleIndex] = Cargo;
		}
	}
}

void UNovaSpacecraftProcessingSystem::Save(FNovaSpacecraft& Spacecraft)
{
	NLOG("UNovaSpacecraftProcessingSystem::Save");

	// Save all cargo to the spacecraft
	for (int32 CompartmentIndex = 0; CompartmentIndex < RealtimeCargo.Num(); CompartmentIndex++)
	{
		FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentIndex];

		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			FNovaSpacecraftCargo& Cargo = Compartment.GetCargo(ModuleIndex);

			Cargo = RealtimeCargo[CompartmentIndex][ModuleIndex];
		}
	}

	RealtimeCargo.Empty();
}

void UNovaSpacecraftProcessingSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	for (auto& IndicesAndState : ModuleStates)
	{
		// Get required constants for each processing module entry
		const int32                                 CompartmentIndex = IndicesAndState.Key.Key;
		const int32                                 ModuleIndex      = IndicesAndState.Key.Value;
		FNovaSpacecraftProcessingSystemModuleState& State            = IndicesAndState.Value;
		const UNovaProcessingModuleDescription*     Module           = State.Module;

		// Search for a group where we belong
		const FNovaModuleGroup* ModuleGroup = GetSpacecraft()->FindModuleGroup(CompartmentIndex, ModuleIndex);
		NCHECK(ModuleGroup);
		if (ModuleGroup)
		{
			// Define processing targets
			float                         MinimumProcessingLeft = FLT_MAX;
			TArray<FNovaSpacecraftCargo*> CurrentInputs;
			TArray<FNovaSpacecraftCargo*> CurrentOutputs;

			// Process cargo for targets
			const TArray<TPair<int32, int32>>& GroupCargoModules =
				GetSpacecraft()->GetAllModules<UNovaCargoModuleDescription>(*ModuleGroup);
			for (const auto& Indices : GroupCargoModules)
			{
				FNovaSpacecraftCargo& Cargo         = RealtimeCargo[Indices.Key][Indices.Value];
				const float           CargoCapacity = GetSpacecraft()->GetCargoCapacity(Indices.Key, Indices.Value);

				// Valid resource input
				if (Cargo.Resource && Module->Inputs.Contains(Cargo.Resource) && Cargo.Amount > 0)
				{
					MinimumProcessingLeft = FMath::Min(MinimumProcessingLeft, Cargo.Amount);
					CurrentInputs.AddUnique(&Cargo);
				}

				// Valid resource output
				else if (Cargo.Resource && Module->Outputs.Contains(Cargo.Resource) && Cargo.Amount < CargoCapacity)
				{
					MinimumProcessingLeft = FMath::Min(MinimumProcessingLeft, CargoCapacity - Cargo.Amount);
					CurrentOutputs.AddUnique(&Cargo);
				}
			}

			// Cancel production when blocked
			if (State.Status == ENovaSpacecraftProcessingSystemStatus::Processing &&
				(CurrentInputs.Num() != Module->Inputs.Num() || CurrentOutputs.Num() != Module->Outputs.Num()))
			{
				State.Status = ENovaSpacecraftProcessingSystemStatus::Blocked;
				NLOG("UNovaSpacecraftProcessingSystem::Update : canceled production for %d,%d ('%s')", CompartmentIndex, ModuleIndex,
					*Module->Name.ToString());
			}

			// Proceed with processing
			if (State.Status == ENovaSpacecraftProcessingSystemStatus::Processing)
			{
				float ResourceDelta = Module->ProcessingRate * (FinalTime - InitialTime).AsSeconds();
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
}

ENovaSpacecraftProcessingSystemStatus UNovaSpacecraftProcessingSystem::GetModuleStatus(int32 CompartmentIndex, int32 ModuleIndex) const
{
	auto StateEntry = ModuleStates.Find(TPair<int32, int32>(CompartmentIndex, ModuleIndex));
	if (StateEntry)
	{
		return StateEntry->Status;
	}
	else
	{
		return ENovaSpacecraftProcessingSystemStatus::Error;
	}
}

void UNovaSpacecraftProcessingSystem::SetModuleActive(int32 CompartmentIndex, int32 ModuleIndex, bool Active)
{
	auto StateEntry = ModuleStates.Find(TPair<int32, int32>(CompartmentIndex, ModuleIndex));
	if (StateEntry)
	{
		bool StateBlocked = StateEntry->Status != ENovaSpacecraftProcessingSystemStatus::Blocked ||
		                    StateEntry->Status != ENovaSpacecraftProcessingSystemStatus::Error;

		StateEntry->Status =
			(Active && !StateBlocked) ? ENovaSpacecraftProcessingSystemStatus::Processing : ENovaSpacecraftProcessingSystemStatus::Stopped;
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

//
// void UNovaSpacecraftProcessingSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//}
