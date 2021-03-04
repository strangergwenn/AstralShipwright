// Nova project - Gwennaël Arbona

#include "NovaPlayerState.h"

#include "Net/UnrealNetwork.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANovaPlayerState::ANovaPlayerState() : Super()
{}

/*----------------------------------------------------
    Networking
----------------------------------------------------*/

void ANovaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaPlayerState, SpacecraftIdentifier);
}
