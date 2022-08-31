// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Spacecraft/NovaSpacecraft.h"

#include "Neutron/Player/NeutronPlayerController.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronPostProcessManager.h"
#include "Neutron/UI/NeutronUI.h"

#include "NovaPlayerController.generated.h"

/** Camera styles supported by shared transitions */
UENUM()
enum class ENovaPlayerCameraState : uint8
{
	Default,
	CinematicSpacecraft,
	CinematicEnvironment,
	CinematicBrake,
	FastForward,
	PhotoMode
};

/** High level post processing targets */
enum class ENovaPostProcessPreset : uint8
{
	Neutral
};

/** Post-process settings */
struct FNeutronPostProcessSetting : public FNeutronPostProcessSettingBase
{
	FNeutronPostProcessSetting()
		: FNeutronPostProcessSettingBase()

		// Custom
	    //, ChromaIntensity(1)

		// Built-in
		, GrainIntensity(0.1f)
		, SceneColorTint(FLinearColor::White)
	{}

	// Custom effects
	// float ChromaIntensity;

	// Built-in effects
	float        GrainIntensity;
	FLinearColor SceneColorTint;
};

/** Player save */
USTRUCT()
struct FNovaPlayerSave
{
	GENERATED_BODY()

	UPROPERTY()
	FNovaSpacecraft Spacecraft;

	UPROPERTY()
	FNovaCredits Credits;

	UPROPERTY()
	TArray<FGuid> UnlockedComponents;
};

/** Camera viewpoint component */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerViewpoint : public AActor
{
	GENERATED_BODY()

public:

	ANovaPlayerViewpoint();

	// Camera animation duration
	UPROPERTY(Category = Nova, EditAnywhere)
	float CameraAnimationDuration;

	// Camera panning amount in degrees over the animation time
	UPROPERTY(Category = Nova, EditAnywhere)
	float CameraPanAmount;

	// Camera tilting amount in degrees over the animation time
	UPROPERTY(Category = Nova, EditAnywhere)
	float CameraTiltAmount;

	// Camera traveling amount in units over the animation time
	UPROPERTY(Category = Nova, EditAnywhere)
	float CameraTravelingAmount;
};

/*----------------------------------------------------
    Player class
----------------------------------------------------*/

/** Default player controller class */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerController : public ANeutronPlayerController
{
	GENERATED_BODY()

public:

	ANovaPlayerController();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	FNovaPlayerSave Save() const;

	void Load(const FNovaPlayerSave& SaveData);

	/** Save the current game */
	void SaveGame();

	/** Load the current game */
	void LoadGame(const FString SaveName);

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void PawnLeavingGame() override;

	virtual void PlayerTick(float DeltaTime) override;

	virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const override;

	virtual bool IsReady() const override;

	virtual void Notify(
		const FText& Text, const FText& Subtext = FText(), ENeutronNotificationType Type = ENeutronNotificationType::Info) override;

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Add a possibly negative amount of credits */
	void ProcessTransaction(FNovaCredits CreditsDelta);

	UFUNCTION(Server, Reliable)
	void ServerProcessTransaction(FNovaCredits CreditsDelta);

	/** Check whether the player can handle this transaction */
	bool CanAffordTransaction(FNovaCredits CreditsDelta) const;

	/** Return the current account balance */
	FNovaCredits GetAccountBalance() const
	{
		return Credits;
	}

	/** Dock the player to a dock with a cutscene */
	void Dock();

	/** Undock the player from the current dock with a cutscene */
	void Undock();

	virtual void OnCameraStateChanged() override;

	virtual bool GetSharedTransitionMenuState(uint8 NewCameraState) override;

	virtual void AnyKey(FKey Key) override;

	/*----------------------------------------------------
	    Server-side save
	----------------------------------------------------*/

public:

	/** Start or restart the game */
	void StartGame(FString SaveName, bool Online = true);

	/** Load the player controller before actors can be created on the server */
	void ClientLoadPlayer();

	/** Create the main player actors on the server */
	UFUNCTION(Server, Reliable)
	void ServerLoadPlayer(const FNovaPlayerSave& PlayerSaveData);

	/** Update the player spacecraft */
	void UpdateSpacecraft(const FNovaSpacecraft& Spacecraft);

	/** Update the player spacecraft */
	UFUNCTION(Server, Reliable)
	void ServerUpdateSpacecraft(const FNovaSpacecraft& Spacecraft);

	/*----------------------------------------------------
	    Progression
	----------------------------------------------------*/

public:

	/** Get which component level is currently available */
	int32 GetComponentUnlockLevel() const;

	/** Check if a particular part is available for unlock */
	bool IsComponentUnlockable(const class UNovaTradableAssetDescription* Asset, FText* Help = nullptr) const;

	/** Check if a particular part is unlocked */
	bool IsComponentUnlocked(const class UNovaTradableAssetDescription* Asset) const;

	/** Get the unlock cost for a level */
	FNovaCredits GetComponentUnlockCost(int32 Level) const;

	/** Get the unlock cost for a part */
	FNovaCredits GetComponentUnlockCost(const class UNovaTradableAssetDescription* Asset) const;

	/** Unlock a part */
	void UnlockComponent(const class UNovaTradableAssetDescription* Asset);

	/*----------------------------------------------------
	    Menus
	----------------------------------------------------*/

public:

	/** Start the photo mode with a transition and wait for ActionName */
	void EnterPhotoMode(FName ActionName);

	/** Exit the photo mode and return to menu */
	void ExitPhotoMode();

	/** Check if we are in photo mode */
	bool IsInPhotoMode() const
	{
		return PhotoModeAction != NAME_None;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:

	// General state
	FName PhotoModeAction;

	// Replicated credits value
	UPROPERTY(Replicated)
	FNovaCredits Credits;

	// Gameplay state
	TMap<ENovaPostProcessPreset, TSharedPtr<FNeutronPostProcessSetting>> PostProcessSettings;

	// List of component IDs for career unlocks
	TArray<FGuid> UnlockedComponents;

	/*----------------------------------------------------
	    Getters
	----------------------------------------------------*/

public:

	/** Get a turntable actor */
	UFUNCTION(Category = Nova, BlueprintCallable)
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const;

	/** Get the player spacecraft */
	const FNovaSpacecraft* GetSpacecraft() const;
};
