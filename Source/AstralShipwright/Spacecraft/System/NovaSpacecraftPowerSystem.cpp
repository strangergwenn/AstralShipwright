// Astral Shipwright - Gwennaël Arbona

#include "NovaSpacecraftPowerSystem.h"

#include "Game/NovaOrbitalSimulationComponent.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSpacecraftPowerSystem::UNovaSpacecraftPowerSystem() : Super()

{
	SetIsReplicatedByDefault(true);
}

/*----------------------------------------------------
    System implementation
----------------------------------------------------*/

void UNovaSpacecraftPowerSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
}

// void UNovaSpacecraftPowerSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
// }
