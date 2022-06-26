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

void UNovaSpacecraftProcessingSystem::Update(FNovaTime InitialTime, FNovaTime FinalTime)
{
	NCHECK(GetOwner()->GetLocalRole() == ROLE_Authority);
}
//
// void UNovaSpacecraftProcessingSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//}
