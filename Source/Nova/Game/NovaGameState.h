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

	/** Set the current game world after it's been spawned */
	void SetGameWorld(class ANovaGameWorld* World);

	/** Set the current area to use */
	void SetCurrentArea(const class UNovaDestination* Area, bool StartDocked);

	/** Get the current game world */
	class ANovaGameWorld* GetGameWorld() const
	{
		return GameWorld;
	}

	/** Get the current destination we are at */
	const class UNovaDestination* GetCurrentArea() const
	{
		return CurrentArea;
	}

	/** Whether spacecraft at this destination should start docked */
	bool ShouldStartDocked() const
	{
		return StartDocked;
	}

	/** Get the current sub-level name to use */
	FName GetCurrentLevelName() const;


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

private:

	// Game world storing all spacecraft
	UPROPERTY(Replicated)
	class ANovaGameWorld*                         GameWorld;

	// Current level-based destination
	UPROPERTY(Replicated)
	const class UNovaDestination*                 CurrentArea;

	// Local state
	bool                                          StartDocked;
};
