// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "NovaGameTypes.h"

#include "Engine/GameInstance.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Net/UnrealNetwork.h"
#include "InputCoreTypes.h"
#include "Online.h"

#include "NovaGameInstance.generated.h"


/** Network states */
UENUM(BlueprintType)
enum class ENovaNetworkState : uint8
{
	Offline,
	Starting,
	OnlineHost,
	OnlineClient,
	Searching,
	Joining,
	JoiningDestroying,
	Ending
};

/** Network errors */
UENUM(BlueprintType)
enum class ENovaNetworkError : uint8
{
	// Session errors
	Success,
	CreateFailed,
	StartFailed,
	DestroyFailed,
	ReadFriendsFailed,
	JoinFriendFailed,
	JoinSessionIsFull,
	JoinSessionDoesNotExist,
	JoinSessionConnectionError,

	// Network errors
	NetDriverError,
	ConnectionLost,
	ConnectionTimeout,
	ConnectionFailed,
	VersionMismatch,

	UnknownError,
};


/** Action to process when a session has been destroyed */
struct FNovaSessionAction
{
	FNovaSessionAction()
	{}

	// Join a new session
	FNovaSessionAction(FOnlineSessionSearchResult NewSessionToJoin)
		: SessionToJoin(NewSessionToJoin)
	{}

	// Create a new offline game
	FNovaSessionAction(FString NewURL)
		: URL(NewURL)
		, Online(false)
	{}

	// Create a new online game
	FNovaSessionAction(FString NewURL, int32 NewMaxNumPlayers, bool NewPublic)
		: URL(NewURL)
		, MaxNumPlayers(NewMaxNumPlayers)
		, Online(true)
		, Public(NewPublic)
	{}

	FOnlineSessionSearchResult                    SessionToJoin;
	FString                                       URL;
	int32                                         MaxNumPlayers;
	bool                                          Online;
	bool                                          Public;
};


// Session delegate
DECLARE_DELEGATE_OneParam(FOnSessionSearchComplete, TArray<FOnlineSessionSearchResult>);

// Friend delegate
DECLARE_DELEGATE_OneParam(FOnFriendSearchComplete, TArray<TSharedRef<FOnlineFriend>>);

// Friend delegate
DECLARE_DELEGATE_OneParam(FOnFriendInviteAccepted, const FOnlineSessionSearchResult&);


/** Game instance class */
UCLASS(ClassGroup = (Nova))
class UNovaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UNovaGameInstance();


	/*----------------------------------------------------
		Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaGameSave> Save() const;

	void Load(TSharedPtr<struct FNovaGameSave> SaveData);

	static void SerializeJson(TSharedPtr<struct FNovaGameSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);


	/*----------------------------------------------------
		Inherited & callbacks
	----------------------------------------------------*/

	virtual void Init() override;

	virtual void Shutdown() override;

	void PreLoadMap(const FString& InMapName);


	/*----------------------------------------------------
		Game save handling
	----------------------------------------------------*/

	/** Start the game from a save file */
	void StartGame(FString SaveName, bool Online = true);

	/** Try loading the game from the save slot if it exists, or create a new one */
	void LoadGame(FString SaveName);

	/** Save the game to the currently loaded slot */
	void SaveGame(bool Synchronous = false);

	/** Save the current save data to the currently loaded slot */
	void SaveGameToFile(bool Synchronous = false);

	/** Check that the current save data is valid */
	bool HasSave() const;

	/** Get the save data for the player */
	TSharedPtr<struct FNovaPlayerSave> GetPlayerSave();

	/** Get the save data for the contract system*/
	TSharedPtr<struct FNovaContractManagerSave> GetContractManagerSave();


	/*----------------------------------------------------
		Game flow
	----------------------------------------------------*/

	/** Get the contract manager */
	UFUNCTION(Category = Nova, BlueprintCallable)
	class UNovaContractManager* GetContractManager() const;

	/** Get the menu manager */
	class UNovaMenuManager* GetMenuManager() const;

	/** Restart the game from the level in save data */
	void SetGameOnline(FString URL, bool Online = true);

	/** Exit the session and go to the main menu */
	void GoToMainMenu();

	/** Change level on the server */
	void ServerTravel(FString URL);


	/*----------------------------------------------------
		Session API
	----------------------------------------------------*/

	/** Get the current online subsystem */
	FString GetOnlineSubsystemName() const;

	/** Are we in an online session */
	bool IsOnline() const;

	/** Are we currently in an unstable state */
	bool IsActive() const;

	/** Start a multiplayer session and open URL as a listen server */
	bool StartSession(FString URL, int32 MaxNumPlayers, bool Public = true);

	/** Finish the online session and open URL as standalone */
	bool EndSession(FString URL);

	/** Search for sessions */
	bool SearchSessions(bool OnLan, FOnSessionSearchComplete Callback);

	/** Join a session */
	bool JoinSearchResult(const FOnlineSessionSearchResult& SearchResult);

	/** Reset the session error */
	void ClearError();


	/*----------------------------------------------------
		Friends API
	----------------------------------------------------*/

	/** Set the callback for accepted friend invitations */
	void SetAcceptedInvitationCallback(FOnFriendInviteAccepted Callback);

	/** Read the list of online friends */
	bool SearchFriends(FOnFriendSearchComplete Callback);

	/** Invite a friend to the session */
	bool InviteFriend(FUniqueNetIdRepl FriendUserId);

	/** Join a friend's session */
	bool JoinFriend(FUniqueNetIdRepl FriendUserId);


protected:

	/*----------------------------------------------------
		Session internals
	----------------------------------------------------*/

	/** Session has been created */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Session has started */
	void OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful);

	/** Session has been destroyed */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Sessions have been found */
	void OnFindSessionsComplete(bool bWasSuccessful);

	/** Session has joined */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);


	/*----------------------------------------------------
		Friends internals
	----------------------------------------------------*/

	/** Friend list is available */
	void OnReadFriendsComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);

	/** Invitation was accepted */
	void OnSessionUserInviteAccepted(bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult);

	/** Friend sessions are available */
	void OnFindFriendSessionComplete(int32 LocalPlayer, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResult);


	/*----------------------------------------------------
		Internals
	----------------------------------------------------*/

	/** Network error */
	void OnNetworkError(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	/** Session error */
	void OnSessionError(ENovaNetworkError Type);

	/** Process an action */
	void ProcessAction(FNovaSessionAction Action);


private:


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/
	
	// Catalog of game assets
	UPROPERTY()
	class UNovaAssetCatalog*                      Catalog;

	// Save manager object
	UPROPERTY()
	class UNovaSaveManager*                       SaveManager;

	// Menu manager object
	UPROPERTY()
	class UNovaMenuManager*                       MenuManager;

	// Contract manager object
	UPROPERTY()
	class UNovaContractManager*                   ContractManager;

	// Save data
	TSharedPtr<struct FNovaGameSave>              CurrentSaveData;
	FString                                       CurrentSaveFileName;
	
	// Data
	TSharedPtr<class FOnlineSessionSettings>      SessionSettings;
	TSharedPtr<class FOnlineSessionSearch>        SessionSearch;
	FString                                       NextURL;
	FNovaSessionAction                            ActionAfterDestroy;
	FNovaSessionAction                            ActionAfterError;

	// Errors
	ENovaNetworkState                             NetworkState;
	ENovaNetworkError                             LastNetworkError;

	// Session created
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;

	// Session started
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;

	// Sessions searched
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;

	// Session joined
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

	// Session destroyed
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;

	// Friend list has been read
	FOnSessionSearchComplete OnSessionListReady;


	// Friend list is available
	FOnReadFriendsListComplete OnReadFriendsListCompleteDelegate;

	// Friend list has been read
	FOnFriendSearchComplete OnFriendListReady;

	// Friend invite accepted or joined manually
	FOnFriendInviteAccepted OnFriendInviteAccepted;

	// Friend invitation accepted through Steam, join that session
	FOnSessionUserInviteAcceptedDelegate OnSessionUserInviteAcceptedDelegate;
	FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;

	// Friend sessions searched
	FOnFindFriendSessionCompleteDelegate OnFindFriendSessionCompleteDelegate;
	FDelegateHandle OnFindFriendSessionCompleteDelegateHandle;


public:

	/*----------------------------------------------------
		Getters
	----------------------------------------------------*/

	class UNovaAssetCatalog* GetCatalog() const
	{
		return Catalog;
	}

	ENovaNetworkState GetNetworkState() const
	{
		return NetworkState;
	}

	ENovaNetworkError GetNetworkError() const
	{
		return LastNetworkError;
	}

	FText GetNetworkStateString() const;

	FText GetNetworkErrorString() const;

};
