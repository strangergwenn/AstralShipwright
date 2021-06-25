// Nova project - Gwennaël Arbona

#pragma once

#include "NovaPostProcessComponent.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"
#include "Nova/UI/NovaUITypes.h"

#include "CoreMinimal.h"
#include "Online.h"
#include "GameFramework/PlayerController.h"

#include "NovaPlayerController.generated.h"

/** Camera styles supported by shared transitions */
UENUM()
enum class ENovaPlayerCameraState : uint8
{
	Default,
	Chase,
	CinematicSpacecraft,
	CinematicEnvironment,
	FastForward,
	PhotoMode
};

/** High level post processing targets */
enum class ENovaPostProcessPreset : uint8
{
	Neutral
};

/** Post-process settings */
struct FNovaPostProcessSetting : public FNovaPostProcessSettingBase
{
	FNovaPostProcessSetting()
		: FNovaPostProcessSettingBase()

		// Custom
	    //, ChromaIntensity(1)

		// Built-in
		, AutoExposureBrightness(1)
		, GrainIntensity(0)
		, SceneColorTint(FLinearColor::White)
	{}

	// Custom effects
	// float ChromaIntensity;

	// Built-in effects
	float        AutoExposureBrightness;
	float        GrainIntensity;
	FLinearColor SceneColorTint;
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

enum class ENovaNetworkError : uint8;

/** Default player controller class */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANovaPlayerController();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaPlayerSave> Save() const;

	void Load(TSharedPtr<struct FNovaPlayerSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaPlayerSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

	virtual void BeginPlay() override;

	virtual void PawnLeavingGame() override;

	virtual void PlayerTick(float DeltaSeconds) override;

	virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const override;

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Add a possibly negative amount of credits */
	void ProcessTransaction(double CreditsDelta);

	UFUNCTION(Server, Reliable)
	void ServerProcessTransaction(double CreditsDelta);

	/** Check whether the player can handle this transaction */
	bool CanAffordTransaction(double CreditsDelta) const;

	/** Return the current account balance */
	double GetAccountBalance() const
	{
		return Credits;
	}

	/** Dock the player to a dock with a cutscene */
	void Dock();

	/** Undock the player from the current dock with a cutscene */
	void Undock();

	/** Run a shared transition with a fade to black on all clients */
	void SharedTransition(ENovaPlayerCameraState NewCameraState, FNovaAsyncAction StartAction = FNovaAsyncAction(),
		FNovaAsyncCondition Condition = FNovaAsyncCondition(), FNovaAsyncAction FinishAction = FNovaAsyncAction());

	/** Signal a client that a shared transition is starting */
	UFUNCTION(Client, Reliable)
	void ClientStartSharedTransition(ENovaPlayerCameraState NewCameraState);

	/** Signal a client that the transition is complete */
	UFUNCTION(Client, Reliable)
	void ClientStopSharedTransition();

	/** Signal the server that the transition is ready */
	UFUNCTION(Server, Reliable)
	void ServerSharedTransitionReady();

	/** Check if loading is currently occurring */
	bool IsLevelStreamingComplete() const;

	/** Set the current camera state */
	void SetCameraState(ENovaPlayerCameraState State);

	/** Get the camera state */
	ENovaPlayerCameraState GetCameraState() const
	{
		return CurrentCameraState;
	}

	/*----------------------------------------------------
	    Server-side save
	----------------------------------------------------*/

public:
	/** Load the player controller before actors can be created on the server */
	void ClientLoadPlayer();

	/** Create the main player actors on the server */
	UFUNCTION(Server, Reliable)
	void ServerLoadPlayer(const FString& SerializedSaveData);

	/** Update the player spacecraft */
	void UpdateSpacecraft(const FNovaSpacecraft& Spacecraft);

	/** Update the player spacecraft */
	UFUNCTION(Server, Reliable)
	void ServerUpdateSpacecraft(const FNovaSpacecraft& Spacecraft);

	/*----------------------------------------------------
	    Game flow
	----------------------------------------------------*/

public:
	/** Start or restart the game */
	void StartGame(FString SaveName, bool Online = true);

	/** Re-start the current level, keeping the save data */
	void SetGameOnline(bool Online = true);

	/** Exit the session and go to the main menu */
	void GoToMainMenu(bool SaveGame);

	/** Exit the game */
	void ExitGame();

	/** Invite a friend to join the game */
	void InviteFriend(TSharedRef<FOnlineFriend> Friend);

	/** Join a friend's game from the menu */
	void JoinFriend(TSharedRef<FOnlineFriend> Friend);

	/** Join a friend's game from an invitation */
	void AcceptInvitation(const FOnlineSessionSearchResult& InviteResult);

	/** Check if the player has a valid pawn */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsReady() const;

	/** Check if the player is currently in a shared transition */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsInSharedTransition() const
	{
		return SharedTransitionActive;
	}

	/*----------------------------------------------------
	    Menus
	----------------------------------------------------*/

public:
	/** Is the player on the main menu */
	bool IsOnMainMenu() const;

	/** Is the player restricted to menus */
	bool IsMenuOnly() const;

	/** Show a text notification on the screen */
	void Notify(const FText& Text, const FText& Subtext = FText(), ENovaNotificationType Type = ENovaNotificationType::Info);

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
	    Input
	----------------------------------------------------*/

public:
	virtual void SetupInputComponent() override;

	/** Any key pressed */
	void AnyKey(FKey Key);

	/** Toggle the main menu */
	void ToggleMenuOrQuit();

#if WITH_EDITOR

	// Test
	void OnJoinRandomFriend(TArray<TSharedRef<FOnlineFriend>> FriendList);
	void OnJoinRandomSession(TArray<FOnlineSessionSearchResult> SessionList);
	void TestJoin();
	void TestActor();

#endif

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

	// Post-processing manager
	UPROPERTY()
	class UNovaPostProcessComponent* PostProcessComponent;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// General state
	ENovaNetworkError      LastNetworkError;
	ENovaPlayerCameraState CurrentCameraState;
	float                  CurrentTimeInCameraState;
	FName                  PhotoModeAction;

	// Replicated credits value
	UPROPERTY(Replicated)
	double Credits;

	// Transitions
	bool                SharedTransitionActive;
	FNovaAsyncAction    SharedTransitionStartAction;
	FNovaAsyncAction    SharedTransitionFinishAction;
	FNovaAsyncCondition SharedTransitionCondition;

	// Gameplay state
	TMap<ENovaPostProcessPreset, TSharedPtr<FNovaPostProcessSetting>> PostProcessSettings;

	/*----------------------------------------------------
	    Getters
	----------------------------------------------------*/

public:
	/** Get the menu manager */
	UFUNCTION(Category = Nova, BlueprintCallable)
	class UNovaMenuManager* GetMenuManager() const
	{
		return GetGameInstance<UNovaGameInstance>()->GetMenuManager();
	}

	/** Get a turntable actor */
	UFUNCTION(Category = Nova, BlueprintCallable)
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const;

	/** Get the player spacecraft */
	const FNovaSpacecraft* GetSpacecraft() const;
};
