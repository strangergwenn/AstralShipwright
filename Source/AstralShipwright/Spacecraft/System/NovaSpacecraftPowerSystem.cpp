// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftPowerSystem.h"
#include "NovaSpacecraftProcessingSystem.h"
#include "Spacecraft/NovaSpacecraftPawn.h"

#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftPowerSystem::UNovaSpacecraftPowerSystem()
	: Super()

	, CurrentPower(0)
	, CurrentPowerProduction(0)
	, CurrentEnergy(0)
	, EnergyCapacity(0)
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftPowerSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const ANovaGameState*                  GameState = GetWorld()->GetGameState<ANovaGameState>();
	const UNovaSpacecraftProcessingSystem* ProcessingSystem =
		GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(GetSpacecraft());
	class UNovaSpacecraftMovementComponent* SpacecraftMovement = Cast<ANovaSpacecraftPawn>(GetOwner())->GetSpacecraftMovement();

	CurrentPower           = 0;
	CurrentPowerProduction = 0;

	// Handle power usage from groups
	for (int32 GroupIndex = 0; GroupIndex < ProcessingSystem->GetProcessingGroupCount(); GroupIndex++)
	{
		CurrentPower -= ProcessingSystem->GetPowerUsage(GroupIndex);
	}

	const auto& Compartments = GetSpacecraft()->Compartments;
	for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
	{
		// Iterate over modules for *production* only, never consumption that we already handled per group
		for (const FNovaCompartmentModule& CompartmentModule : Compartments[CompartmentIndex].Modules)
		{
			const UNovaProcessingModuleDescription* Module = Cast<UNovaProcessingModuleDescription>(CompartmentModule.Description);
			if (::IsValid(Module))
			{
				CurrentPower += FMath::Max(-Module->Power, 0);
			}
		}

		// Iterate over equipment
		for (const UNovaEquipmentDescription* Equipment : Compartments[CompartmentIndex].Equipment)
		{
			// Solar panels
			const UNovaPowerEquipmentDescription* PowerEquipment = Cast<UNovaPowerEquipmentDescription>(Equipment);
			if (PowerEquipment)
			{
				CurrentPower += PowerEquipment->Power;
				CurrentPowerProduction += PowerEquipment->Power;
			}

			// Mining rig
			const UNovaMiningEquipmentDescription* MiningEquipment = Cast<UNovaMiningEquipmentDescription>(Equipment);
			if (MiningEquipment && ProcessingSystem->IsMiningRigActive())
			{
				CurrentPower -= MiningEquipment->Power;
			}

			// Mast
			const UNovaRadioMastDescription* MastEquipment = Cast<UNovaRadioMastDescription>(Equipment);
			if (MastEquipment && IsValid(GameState) && IsValid(GameState->GetCurrentArea()) && GameState->GetCurrentArea()->IsInSpace &&
				IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() >= ENovaMovementState::Orbiting)
			{
				CurrentPower -= MastEquipment->Power;
			}
		}
	}

	// Handle batteries
	EnergyCapacity += CurrentPower * (FinalTime - InitialTime).AsHours();
	EnergyCapacity = FMath::Clamp(EnergyCapacity, 0.0, EnergyCapacity);
}

void UNovaSpacecraftPowerSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentPower);
	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentPowerProduction);
	DOREPLIFETIME(UNovaSpacecraftPowerSystem, CurrentEnergy);
}
