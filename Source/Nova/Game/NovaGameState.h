// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NovaGameState.generated.h"


/** Replicated game state class */
UCLASS(ClassGroup = (Nova))
class ANovaGameState : public AGameStateBase
{
	GENERATED_BODY()
		
public:

	ANovaGameState();


	/*----------------------------------------------------
		Gameplay
	----------------------------------------------------*/

	/** Set the current area to use */
	void SetCurrentArea(const class UNovaDestination* Area);

	/** Get the current destination we are at */
	const class UNovaDestination* GetCurrentArea() const
	{
		return CurrentArea;
	}

	/** Get the current sub-level name to use */
	FName GetCurrentLevelName() const;


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

private:

	// Game state
	UPROPERTY(Replicated)
	const class UNovaDestination*                 CurrentArea;
};
