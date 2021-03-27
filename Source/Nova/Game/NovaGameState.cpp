// Nova project - Gwennaël Arbona

#include "NovaGameState.h"
#include "NovaArea.h"

#include "Nova/Player/NovaPlayerState.h"

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
	return CurrentArea ? CurrentArea->LevelName : NAME_None;
}

TArray<FGuid> ANovaGameState::GetPlayerSpacecraftIdentifiers() const
{
	TArray<FGuid> Result;

	for (const APlayerState* PlayerStateBase : PlayerArray)
	{
		const ANovaPlayerState* PlayerState = Cast<ANovaPlayerState>(PlayerStateBase);
		if (IsValid(PlayerState))
		{
			Result.Add(PlayerState->GetSpacecraftIdentifier());
		}
	}

	return Result;
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
