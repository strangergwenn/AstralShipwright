// Nova project - Gwennaël Arbona

#pragma once

#include "NovaPostProcessComponent.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/UI/NovaUITypes.h"

#include "CoreMinimal.h"
#include "Online.h"
#include "GameFramework/PlayerController.h"

#include "NovaPlayerController.generated.h"


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
	//float ChromaIntensity;

	// Built-in effects
	float AutoExposureBrightness;
	float GrainIntensity;
	FLinearColor SceneColorTint;
};


/** Camera viewpoint */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerViewpoint : public AActor
{
	GENERATED_BODY()

public:

	ANovaPlayerViewpoint();
};

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

	static void SerializeJson(TSharedPtr<struct FNovaPlayerSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);


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

	/** Run a shared transition with a fade to black on all clients */
	void SharedTransition(bool CutsceneMode, FSimpleDelegate StartCallback, FSimpleDelegate FinishCallback = FSimpleDelegate());

	/** Signal a client that a shared transition is starting */
	UFUNCTION(Client, Reliable)
	void ClientStartSharedTransition(bool CutsceneMode);

	/** Signal a client that the transition is complete */
	UFUNCTION(Client, Reliable)
	void ClientStopSharedTransition();

	/** Signal the server that the transition is ready */
	UFUNCTION(Server, Reliable)
	void ServerSharedTransitionReady();

	/** Check if loading is currently ocurring */
	bool IsStreamingLevel() const;

	/** Dock the player to a dock with a cutscene */
	void Dock();

	/** Undock the player from the current dock with a cutscene */
	void Undock();


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
	void GoToMainMenu();

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


	/*----------------------------------------------------
		Menus
	----------------------------------------------------*/

public:

	/** Is the player on the main menu */
	bool IsOnMainMenu() const;

	/** Is the player restricted to menus */
	bool IsMenuOnly() const;

	/** Show a text notification on the screen */
	void Notify(FText Text, ENovaNotificationType Type);


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

	// Travel state
	ENovaNetworkError                             LastNetworkError;

	// Transitions
	bool                                          IsInSharedTransition;
	FSimpleDelegate                               OnSharedTransitionStarted;
	FSimpleDelegate                               OnSharedTransitionFinished;

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
	class ANovaSpacecraftPawn* GetSpacecraftPawn() const
	{
		return GetPawn<ANovaSpacecraftPawn>();
	}

	/** Get the game world actor */
	UFUNCTION(Category = Nova, BlueprintCallable)
	class ANovaGameWorld* GetGameWorld() const;

	/** Get the player spacecraft */
	TSharedPtr<struct FNovaSpacecraft> GetSpacecraft() const;

};
