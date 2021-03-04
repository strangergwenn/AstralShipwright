// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NovaPlayerState.generated.h"

/** Replicated player state class */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ANovaPlayerState();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Share the identifier for the player spacecraft */
	void SetSpacecraftIdentifier(FGuid Identifier)
	{
		SpacecraftIdentifier = Identifier;
	}

	/** Return the identifier for the player spacecraft */
	const FGuid GetSpacecraftIdentifier() const
	{
		return SpacecraftIdentifier;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Player world storing all spacecraft
	UPROPERTY(Replicated)
	FGuid SpacecraftIdentifier;
};
