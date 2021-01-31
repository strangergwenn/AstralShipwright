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

	/** Get all players to leave a zone */
	void LeaveArea();

	
	/*----------------------------------------------------
		Level loading
	----------------------------------------------------*/

protected:
	
	/** Load a streaming level */
	bool LoadStreamingLevel(FName SectorLevel);

	/** Unload a streaming level */
	void UnloadStreamingLevel(FName SectorLevel);

	/** Callback for a loaded streaming level */
	UFUNCTION(BlueprintCallable, Category = GameMode)
	void OnLevelLoaded();

	/** Callback for an unloaded streaming level */
	UFUNCTION(BlueprintCallable, Category = GameMode)
	void OnLevelUnLoaded();


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

private:

	// Game state
	bool                                          IsLoadingStreamingLevel;
	bool                                          IsUnloadingStreamingLevel;
	int32                                         CurrentStreamingLevelIndex;
	

	/*----------------------------------------------------
		Getters
	----------------------------------------------------*/

public:

	/** Check if we're loading or unloading a level */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsLoadingUnloadingLevel() const
	{
		return IsLoadingStreamingLevel || IsUnloadingStreamingLevel;
	}
};
