// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftCrewSystem.h"
#include "NovaSpacecraftProcessingSystem.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftTypes.h"

#include "Neutron/System/NeutronAssetManager.h"

#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "UNovaSpacecraftCrewSystem"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftCrewSystem::UNovaSpacecraftCrewSystem() : Super()
{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftCrewSystem::Load(const FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftCrewSystem::Load");
}

void UNovaSpacecraftCrewSystem::Save(FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftCrewSystem::Save");
}

void UNovaSpacecraftCrewSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
}

int32 UNovaSpacecraftCrewSystem::GetTotalCrew() const
{
	int32 TotalCrewCount = 0;

	const FNovaSpacecraft* Spacecraft = GetSpacecraft();

	if (Spacecraft)
	{
		for (int32 CompartmentIndex = 0; CompartmentIndex < Spacecraft->Compartments.Num(); CompartmentIndex++)
		{
			const FNovaCompartment& Compartment = Spacecraft->Compartments[CompartmentIndex];

			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				const UNovaModuleDescription* Module = Compartment.Modules[ModuleIndex].Description;
				if (Module && Module->CrewEffect > 0)
				{
					TotalCrewCount += Module->CrewEffect;
				}
			}

			for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
			{
				const UNovaEquipmentDescription* Equipment = Compartment.Equipment[EquipmentIndex];
				if (Equipment && Equipment->CrewEffect > 0)
				{
					TotalCrewCount += Equipment->CrewEffect;
				}
			}
		}
	}

	return TotalCrewCount;
}

int32 UNovaSpacecraftCrewSystem::GetRequiredCrew(int32 ProcessingGroupIndex) const
{
	const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetProcessingSystem();

	return ProcessingSystem->GetProcessingGroupCrew(ProcessingGroupIndex, false);
}

int32 UNovaSpacecraftCrewSystem::GetBusyCrew(int32 ProcessingGroupIndex) const
{
	const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetProcessingSystem();

	return ProcessingSystem->GetProcessingGroupCrew(ProcessingGroupIndex, true);
}

int32 UNovaSpacecraftCrewSystem::GetTotalBusyCrew() const
{
	const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetProcessingSystem();

	int32 Count = 0;
	for (int32 ProcessingGroupIndex = 0; ProcessingGroupIndex < ProcessingSystem->GetProcessingGroupCount(); ProcessingGroupIndex++)
	{
		Count += GetBusyCrew(ProcessingGroupIndex);
	}

	return Count;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

const UNovaSpacecraftProcessingSystem* UNovaSpacecraftCrewSystem::GetProcessingSystem() const
{
	const FNovaSpacecraft* Spacecraft = GetSpacecraft();
	const ANovaGameState*  GameState  = GetWorld()->GetGameState<ANovaGameState>();

	return GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(Spacecraft);
}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

// void UNovaSpacecraftCrewSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
// }

#undef LOCTEXT_NAMESPACE
