// Nova project - Gwennaël Arbona

#include "NovaGameState.h"
#include "NovaArea.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaGameState::ANovaGameState() : Super(), GameWorld(nullptr), CurrentArea(nullptr)
{}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaGameState::SetGameWorld(class ANovaGameWorld* World)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	GameWorld = World;
}

void ANovaGameState::SetCurrentArea(const UNovaArea* Area, bool Docked)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	CurrentArea = Area;
	StartDocked = Docked;
}

FName ANovaGameState::GetCurrentLevelName() const
{
	NCHECK(CurrentArea);
	return CurrentArea->LevelName;
}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

void ANovaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaGameState, GameWorld);
	DOREPLIFETIME(ANovaGameState, CurrentArea);
}
