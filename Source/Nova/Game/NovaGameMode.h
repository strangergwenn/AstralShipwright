// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovaGameMode.generated.h"

/** Game state */
enum class ENovaGameStateIdentifier : uint8
{
	// Idle states
	Area,
	Orbit,
	FastForward,

	// Exit maneuvers
	DepartureProximity,
	DepartureCoast,

	// Arrival maneuvers
	ArrivalIntro,
	ArrivalCoast,
	ArrivalProximity
};

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

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void PostLogin(class APlayerController* Player) override;

	virtual void Logout(class AController* Player) override;

	virtual class UClass* GetDefaultPawnClassForController_Implementation(class AController* InController) override;

	virtual AActor* ChoosePlayerStart_Implementation(class AController* Player) override;

	void Tick(float DeltaTime) override;

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	/** Run the time-skipping process in a shared transition */
	void FastForward();

	/** Check whether we can fast-forward */
	bool CanFastForward() const;

	/** Reset all spacecraft movement */
	void ResetSpacecraft();

	/** Change the current area */
	void ChangeArea(const class UNovaArea* Area);

	/** Change the current area to orbit*/
	void ChangeAreaToOrbit();

	/** Check if we are in orbit */
	bool IsInOrbit() const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Set up the game state machine */
	void InitializeStateMachine();

	/** Run the game state machine */
	void ProcessStateMachine();

	/** Load a streaming level */
	bool LoadStreamingLevel(const class UNovaArea* Area, FSimpleDelegate Callback = FSimpleDelegate());

	/** Unload a streaming level */
	void UnloadStreamingLevel(const class UNovaArea* Area, FSimpleDelegate Callback = FSimpleDelegate());

	/** Callback for a loaded streaming level */
	UFUNCTION()
	void OnLevelLoaded();

	/** Callback for an unloaded streaming level */
	UFUNCTION()
	void OnLevelUnLoaded();

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	/** Area to use for orbit */
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	const class UNovaArea* OrbitArea;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Game state
	ENovaGameStateIdentifier                                             DesiredStateIdentifier;
	ENovaGameStateIdentifier                                             CurrentStateIdentifier;
	TMap<ENovaGameStateIdentifier, TSharedPtr<class FNovaGameModeState>> StateMap;

	// Level streaming state
	int32           CurrentStreamingLevelIndex;
	FSimpleDelegate OnLevelLoadedCallback;
	FSimpleDelegate OnLevelUnloadedCallback;
};
