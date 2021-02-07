// Nova project - Gwennaël Arbona

#include "NovaGameState.h"
#include "NovaDestination.h"

#include "Net/UnrealNetwork.h"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

ANovaGameState::ANovaGameState()
	: Super()
	, CurrentArea(nullptr)
{

}


/*----------------------------------------------------
	Gameplay
----------------------------------------------------*/

void ANovaGameState::SetCurrentArea(const class UNovaDestination* Area)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	CurrentArea = Area;
}

FName ANovaGameState::GetCurrentLevelName() const
{
	return CurrentArea->LevelName;
}


/*----------------------------------------------------
	Networking
----------------------------------------------------*/

void ANovaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameState, CurrentArea);
}

