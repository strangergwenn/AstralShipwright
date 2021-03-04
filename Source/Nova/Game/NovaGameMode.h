// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovaGameMode.generated.h"

/** Default game mode class */
UCLASS(ClassGroup = (Nova))
class ANovaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANovaGameMode();

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void InitGameState() override;

	virtual void StartPlay() override;

	virtual void PostLogin(class APlayerController* Player) override;

	virtual void Logout(class AController* Player) override;

	virtual class UClass* GetDefaultPawnClassForController_Implementation(class AController* InController) override;

	virtual AActor* ChoosePlayerStart_Implementation(class AController* Player) override;

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	/** Reset the area, moving all ships to player starts */
	void ResetArea();

	/** Have all players fade to black, play the exit cutscene and switch area */
	void ChangeArea(const class UNovaArea* Area);

	/*----------------------------------------------------
	    Level loading
	----------------------------------------------------*/

protected:
	/** Load a streaming level */
	bool LoadStreamingLevel(const class UNovaArea* Area, bool StartDocked = false, FSimpleDelegate Callback = FSimpleDelegate());

	/** Unload a streaming level */
	void UnloadStreamingLevel(const class UNovaArea* Area, FSimpleDelegate Callback = FSimpleDelegate());

	/** Callback for a loaded streaming level */
	UFUNCTION()
	void OnLevelLoaded();

	/** Callback for an unloaded streaming level */
	UFUNCTION()
	void OnLevelUnLoaded();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Game state
	int32           CurrentStreamingLevelIndex;
	FSimpleDelegate OnLevelLoadedCallback;
	FSimpleDelegate OnLevelUnloadedCallback;
};
