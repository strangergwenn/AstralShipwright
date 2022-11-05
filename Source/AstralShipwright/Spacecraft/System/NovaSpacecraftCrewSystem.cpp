// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftCrewSystem.h"
#include "NovaSpacecraftProcessingSystem.h"

#include "Game/NovaGameTypes.h"
#include "Player/NovaPlayerController.h"
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

	CurrentTimeSincePayday = 0;
}

void UNovaSpacecraftCrewSystem::Save(FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	NLOG("UNovaSpacecraftCrewSystem::Save");
}

void UNovaSpacecraftCrewSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);

	const FNovaTime PaydayPeriod = FNovaTime::FromDays(1);

	// Handle pay
	CurrentTimeSincePayday += (FinalTime - InitialTime);
	if (CurrentTimeSincePayday > PaydayPeriod)
	{
		int32 UpdatedCrewCount;
		int32 CurrentCrewCount = GetCurrentCrew();

		// Try to pay each employee
		for (UpdatedCrewCount = 0; UpdatedCrewCount < CurrentCrewCount; UpdatedCrewCount++)
		{
			FNovaCredits UnitCost = -GetDailyCostPerCrew();
			if (GetPC()->CanAffordTransaction(UnitCost))
			{
				GetPC()->ProcessTransaction(UnitCost);
			}
			else
			{
				break;
			}
		}

		// Fire and notify
		if (UpdatedCrewCount < CurrentCrewCount)
		{
			GetPC()->SetCurrentCrew(UpdatedCrewCount);
			GetPC()->Notify(LOCTEXT("CrewFired", "Crew not paid"),
				FText::FormatNamed(LOCTEXT("CrewFiredDetails",
									   "Crew downsized from {previous} to {current}, employees dismissed"),
					TEXT("previous"), FText::AsNumber(CurrentCrewCount), TEXT("current"), FText::AsNumber(UpdatedCrewCount)));
		}

		CurrentTimeSincePayday -= PaydayPeriod;
	}
}

int32 UNovaSpacecraftCrewSystem::GetCurrentCrew() const
{
	return FMath::Clamp(GetPC()->GetCurrentCrew(), 0, GetCrewCapacity());
}

int32 UNovaSpacecraftCrewSystem::GetRequiredCrew(int32 ProcessingGroupIndex) const
{
	const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetProcessingSystem();

	return ProcessingSystem->GetProcessingGroupCrew(ProcessingGroupIndex, false);
}

int32 UNovaSpacecraftCrewSystem::GetTotalRequiredCrew() const
{
	const UNovaSpacecraftProcessingSystem* ProcessingSystem = GetProcessingSystem();

	int32 Count = 0;
	for (int32 ProcessingGroupIndex = 0; ProcessingGroupIndex < ProcessingSystem->GetProcessingGroupCount(); ProcessingGroupIndex++)
	{
		Count += GetRequiredCrew(ProcessingGroupIndex);
	}

	return Count;
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

int32 UNovaSpacecraftCrewSystem::GetCrewCapacity() const
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

FNovaCredits UNovaSpacecraftCrewSystem::GetDailyCostPerCrew() const
{
	return FNovaCredits(10);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

const UNovaSpacecraftProcessingSystem* UNovaSpacecraftCrewSystem::GetProcessingSystem() const
{
	const FNovaSpacecraft* Spacecraft = GetSpacecraft();
	const ANovaGameState*  GameState  = GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);

	return GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(Spacecraft);
}

ANovaPlayerController* UNovaSpacecraftCrewSystem::GetPC() const
{
	ANovaSpacecraftPawn* SpacecraftPawn = Cast<ANovaSpacecraftPawn>(GetOwner());
	NCHECK(SpacecraftPawn);

	ANovaPlayerController* PC = Cast<ANovaPlayerController>(SpacecraftPawn->GetController());
	NCHECK(PC);

	return PC;
}

#undef LOCTEXT_NAMESPACE
