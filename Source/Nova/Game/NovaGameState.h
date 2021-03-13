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
	void SetCurrentArea(const class UNovaArea* Area, bool StartDocked);

	/** Get the current game world */
	class ANovaGameWorld* GetGameWorld() const
	{
		return GameWorld;
	}

	/** Get the current area we are at */
	const class UNovaArea* GetCurrentArea() const
	{
		return CurrentArea;
	}

	/** Whether spacecraft at this area should start docked */
	bool ShouldStartDocked() const
	{
		return StartDocked;
	}

	/** Get the current sub-level name to use */
	FName GetCurrentLevelName() const;

	/** Get the list of all player spacecraft identifiers */
	TArray<FGuid> GetPlayerSpacecraftIdentifiers() const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Game world storing all spacecraft
	UPROPERTY(Replicated)
	class ANovaGameWorld* GameWorld;

	// Current level-based area
	UPROPERTY(Replicated)
	const class UNovaArea* CurrentArea;

	// Local state
	bool StartDocked;
};
