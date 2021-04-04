// Nova project - Gwennaël Arbona

#include "NovaGameInstance.h"

#include "NovaAssetCatalog.h"
#include "NovaGameMode.h"
#include "NovaGameState.h"
#include "NovaGameUserSettings.h"
#include "NovaContractManager.h"
#include "NovaSaveManager.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaGameViewportClient.h"

#include "Nova/UI/NovaUI.h"

#include "Nova/Nova.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "UNovaGameInstance"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaGameInstance::UNovaGameInstance() : Super(), CurrentSaveData(nullptr), NetworkState(ENovaNetworkState::Offline)
{
	// Session callbacks
	OnCreateSessionCompleteDelegate  = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNovaGameInstance::OnCreateSessionComplete);
	OnStartSessionCompleteDelegate   = FOnStartSessionCompleteDelegate::CreateUObject(this, &UNovaGameInstance::OnStartOnlineGameComplete);
	OnFindSessionsCompleteDelegate   = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNovaGameInstance::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate    = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNovaGameInstance::OnJoinSessionComplete);
	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNovaGameInstance::OnDestroySessionComplete);

	// Friends callbacks
	OnReadFriendsListCompleteDelegate = FOnReadFriendsListComplete::CreateUObject(this, &UNovaGameInstance::OnReadFriendsComplete);
	OnSessionUserInviteAcceptedDelegate =
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UNovaGameInstance::OnSessionUserInviteAccepted);
	OnFindFriendSessionCompleteDelegate =
		FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &UNovaGameInstance::OnFindFriendSessionComplete);
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaGameSave
{
	TSharedPtr<struct FNovaPlayerSave>          PlayerData;
	TSharedPtr<struct FNovaGameStateSave>       GameStateData;
	TSharedPtr<struct FNovaContractManagerSave> ContractManagerData;
};

TSharedPtr<FNovaGameSave> UNovaGameInstance::Save(const ANovaPlayerController* PC) const
{
	TSharedPtr<FNovaGameSave> Save = CurrentSaveData;

	// Save the player
	Save->PlayerData = PC->Save();

	// Save the world
	if (PC->GetLocalRole() == ROLE_Authority)
	{
		ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		if (IsValid(GameState))
		{
			Save->GameStateData = GameState->Save();
		}
	}

	// Save contracts
	Save->ContractManagerData = ContractManager->Save();

	return Save;
}

void UNovaGameInstance::Load(TSharedPtr<FNovaGameSave> SaveData)
{
	CurrentSaveData = SaveData;

	// Only load contracts right away, other classes will fetch their stuff when they need it
	ContractManager->Load(GetContractManagerSave());
}

void UNovaGameInstance::SerializeJson(TSharedPtr<FNovaGameSave>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		TSharedPtr<FJsonObject> PlayerJsonData = MakeShared<FJsonObject>();
		ANovaPlayerController::SerializeJson(SaveData->PlayerData, PlayerJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("Player", PlayerJsonData);

		TSharedPtr<FJsonObject> GameStateJsonData = MakeShared<FJsonObject>();
		ANovaGameState::SerializeJson(SaveData->GameStateData, GameStateJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("GameState", GameStateJsonData);

		TSharedPtr<FJsonObject> ContractManagerJsonData = MakeShared<FJsonObject>();
		UNovaContractManager::SerializeJson(SaveData->ContractManagerData, ContractManagerJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("ContractManager", ContractManagerJsonData);
	}
	else
	{
		SaveData = MakeShared<FNovaGameSave>();

		TSharedPtr<FJsonObject> PlayerJsonData = JsonData->GetObjectField("Player");
		ANovaPlayerController::SerializeJson(SaveData->PlayerData, PlayerJsonData, ENovaSerialize::JsonToData);

		TSharedPtr<FJsonObject> GameStateJsonData = JsonData->GetObjectField("GameState");
		ANovaGameState::SerializeJson(SaveData->GameStateData, GameStateJsonData, ENovaSerialize::JsonToData);

		TSharedPtr<FJsonObject> ContractManagerJsonData = JsonData->GetObjectField("ContractManager");
		UNovaContractManager::SerializeJson(SaveData->ContractManagerData, ContractManagerJsonData, ENovaSerialize::JsonToData);
	}
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaGameInstance::Init()
{
	Super::Init();

	// Apply user settings
	UNovaGameUserSettings* GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
	GameUserSettings->ApplyCustomGraphicsSettings();

	// Setup connection screen
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UNovaGameInstance::PreLoadMap);

	// Setup network errors
	GetEngine()->OnNetworkFailure().AddUObject(this, &UNovaGameInstance::OnNetworkError);

	// Setup friends invites
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		OnSessionUserInviteAcceptedDelegateHandle =
			Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
	}

	// Get asset inventory
	Catalog = NewObject<UNovaAssetCatalog>(this, UNovaAssetCatalog::StaticClass(), TEXT("AssetCatalog"));
	NCHECK(Catalog);
	Catalog->Initialize();

	// Create save manager
	SaveManager = NewObject<UNovaSaveManager>(this, UNovaSaveManager::StaticClass(), TEXT("SaveManager"));
	NCHECK(SaveManager);

	// Create the menu manager
	MenuManager = NewObject<UNovaMenuManager>(this, UNovaMenuManager::StaticClass(), TEXT("MenuManager"));
	NCHECK(MenuManager);
	MenuManager->Initialize(this);

	// Create the contract manager
	ContractManager = NewObject<UNovaContractManager>(this, UNovaContractManager::StaticClass(), TEXT("ContractManager"));
	NCHECK(ContractManager);
	ContractManager->Initialize(this);
}

void UNovaGameInstance::Shutdown()
{
	// Clean up friends invites
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}

	Super::Shutdown();
}

void UNovaGameInstance::PreLoadMap(const FString& InMapName)
{
	UNovaGameViewportClient* Viewport = Cast<UNovaGameViewportClient>(GetWorld()->GetGameViewport());
	if (Viewport)
	{
		Viewport->ShowLoadingScreen();
	}
}

/*----------------------------------------------------
    Game save handling
----------------------------------------------------*/

void UNovaGameInstance::StartGame(FString SaveName, bool Online)
{
	NLOG("UNovaGameInstance::StartGame");

	LoadGame(SaveName);

	SetGameOnline(ENovaConstants::DefaultLevel, Online);
}

void UNovaGameInstance::LoadGame(FString SaveName)
{
	NLOG("UNovaGameInstance::LoadGame : loading from '%s'", *SaveName);

	CurrentSaveData     = nullptr;
	CurrentSaveFileName = SaveName;

	// Read and de-serialize all data, without actually loading objects
	if (SaveManager->DoesSaveExist(SaveName))
	{
		CurrentSaveData = SaveManager->LoadGame(SaveName);
	}
	else
	{
		CurrentSaveData = MakeShared<FNovaGameSave>();
	}
	NCHECK(CurrentSaveData);

	Load(CurrentSaveData);
}

void UNovaGameInstance::SaveGame(ANovaPlayerController* PC, bool Synchronous)
{
	NLOG("UNovaGameInstance::SaveGame : synchronous %d", Synchronous);

	NCHECK(PC);

	// Get an updated save
	CurrentSaveData = Save(PC);

	// Write to file
	SaveGameToFile(Synchronous);
	PC->Notify(LOCTEXT("SavedGame", "Game saved"), ENovaNotificationType::Saved);
}

void UNovaGameInstance::SaveGameToFile(bool Synchronous)
{
	NLOG("UNovaGameInstance::SaveGameToFile : synchronous %d", Synchronous);

	if (CurrentSaveData.IsValid())
	{
		if (Synchronous)
		{
			SaveManager->SaveGame(CurrentSaveFileName, CurrentSaveData);
		}
		else
		{
			SaveManager->SaveGameAsync(CurrentSaveFileName, CurrentSaveData);
		}
	}
}

bool UNovaGameInstance::HasSave() const
{
	return CurrentSaveData.IsValid();
}

TSharedPtr<FNovaPlayerSave> UNovaGameInstance::GetPlayerSave()
{
	NCHECK(CurrentSaveData);
	return CurrentSaveData->PlayerData;
}

TSharedPtr<struct FNovaGameStateSave> UNovaGameInstance::GetWorldSave()
{
	NCHECK(CurrentSaveData);
	return CurrentSaveData->GameStateData;
}

TSharedPtr<FNovaContractManagerSave> UNovaGameInstance::GetContractManagerSave()
{
	NCHECK(CurrentSaveData);
	return CurrentSaveData->ContractManagerData;
}

/*----------------------------------------------------
    Game flow
----------------------------------------------------*/

UNovaContractManager* UNovaGameInstance::GetContractManager() const
{
	return ContractManager;
}

UNovaMenuManager* UNovaGameInstance::GetMenuManager() const
{
	return MenuManager;
}

void UNovaGameInstance::SetGameOnline(FString URL, bool Online)
{
	NLOG("UNovaGameInstance::SetGameOnline : '%s', online = %d", *URL, Online);

	ClearError();
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetFirstLocalPlayerController());

	// We want to be online and presumably aren't, start a new online session and then travel to the level
	if (Online)
	{
		StartSession(URL, ENovaConstants::MaxPlayerCount, true);
	}

	// We are on the main menu and are staying offline, simply travel there
	else if (PC->IsOnMainMenu())
	{
		PC->ClientTravel(URL, ETravelType::TRAVEL_Absolute, false);
	}

	// We want to be offline and presumably aren't, kill the session and then travel to the level
	else
	{
		EndSession(URL);
	}
}

void UNovaGameInstance::GoToMainMenu()
{
	NLOG("UNovaGameInstance::GoToMainMenu");

	CurrentSaveData = nullptr;

	UGameplayStatics::OpenLevel(GetWorld(), FName("MenuMap"), true);
}

void UNovaGameInstance::ServerTravel(FString URL)
{
	GetWorld()->ServerTravel(URL + TEXT("?listen"), true);
}

/*----------------------------------------------------
    Session
----------------------------------------------------*/

FString UNovaGameInstance::GetOnlineSubsystemName() const
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		return OnlineSub->GetOnlineServiceName().ToString();
	}

	return FString("Null");
}

bool UNovaGameInstance::IsOnline() const
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			return Sessions->IsPlayerInSession(GameSessionName, *UserId);
		}
	}

	return false;
}

bool UNovaGameInstance::IsActive() const
{
	return (NetworkState != ENovaNetworkState::Offline && NetworkState != ENovaNetworkState::OnlineClient &&
			NetworkState != ENovaNetworkState::OnlineHost);
}

bool UNovaGameInstance::StartSession(FString URL, int32 MaxNumPlayers, bool Public)
{
	NLOG("UNovaGameInstance::CreateSession");

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			ActionAfterError = FNovaSessionAction(GetWorld()->GetName());

			// Player is already in session, leave it
			if (Sessions->IsPlayerInSession(GameSessionName, *UserId))
			{
				ActionAfterDestroy = FNovaSessionAction(URL, MaxNumPlayers, Public);
				NetworkState       = ENovaNetworkState::JoiningDestroying;

				OnDestroySessionCompleteDelegateHandle =
					Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
				return Sessions->DestroySession(GameSessionName);
			}

			// Player isn't in a session, create it
			else
			{
				SessionSettings = MakeShared<FOnlineSessionSettings>();

				SessionSettings->NumPublicConnections             = MaxNumPlayers;
				SessionSettings->NumPrivateConnections            = 0;
				SessionSettings->bIsLANMatch                      = false;
				SessionSettings->bUsesPresence                    = true;
				SessionSettings->bAllowInvites                    = true;
				SessionSettings->bAllowJoinInProgress             = true;
				SessionSettings->bShouldAdvertise                 = Public;
				SessionSettings->bAllowJoinViaPresence            = true;
				SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;

				NextURL = URL;
				SessionSettings->Set(SETTING_MAPNAME, URL, EOnlineDataAdvertisementType::ViaOnlineService);

				NetworkState = ENovaNetworkState::Starting;

				// Start
				OnCreateSessionCompleteDelegateHandle =
					Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
				return Sessions->CreateSession(*UserId, GameSessionName, *SessionSettings);
			}
		}
	}

	return false;
}

bool UNovaGameInstance::EndSession(FString URL)
{
	NLOG("UNovaGameInstance::EndSession");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		// Destroy
		if (Sessions.IsValid())
		{
			ActionAfterError   = FNovaSessionAction(GetWorld()->GetName());
			ActionAfterDestroy = FNovaSessionAction(URL);
			NetworkState       = ENovaNetworkState::Ending;

			OnDestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
			return Sessions->DestroySession(GameSessionName);
		}
	}

	return false;
}

bool UNovaGameInstance::SearchSessions(bool OnLan, FOnSessionSearchComplete Callback)
{
	NLOG("UNovaGameInstance::SearchSessions");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	OnSessionListReady = Callback;

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Start searching
		if (Sessions.IsValid() && UserId.IsValid())
		{
			SessionSearch = MakeShared<FOnlineSessionSearch>();

			SessionSearch->bIsLanQuery      = OnLan;
			SessionSearch->MaxSearchResults = 10;
			SessionSearch->PingBucketSize   = 100;
			SessionSearch->TimeoutInSeconds = 3;

			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

			NetworkState = ENovaNetworkState::Searching;

			// Start
			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();
			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			return Sessions->FindSessions(*UserId, SearchSettingsRef);
		}
	}

	return false;
}

bool UNovaGameInstance::JoinSearchResult(const FOnlineSessionSearchResult& SearchResult)
{
	NLOG("UNovaGameInstance::JoinSearchResult");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Start joining
		if (Sessions.IsValid() && UserId.IsValid())
		{
			// Player is already in session, leave it
			if (Sessions->IsPlayerInSession(GameSessionName, *UserId))
			{
				OnDestroySessionCompleteDelegateHandle =
					Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

				ActionAfterError   = FNovaSessionAction(GetWorld()->GetName());
				ActionAfterDestroy = FNovaSessionAction(SearchResult);

				NetworkState = ENovaNetworkState::JoiningDestroying;

				return Sessions->DestroySession(GameSessionName);
			}

			// Join directly
			else
			{
				NetworkState = ENovaNetworkState::Joining;

				OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

				return Sessions->JoinSession(*Player->GetPreferredUniqueNetId(), GameSessionName, SearchResult);
			}
		}
	}

	return false;
}

void UNovaGameInstance::ClearError()
{
	LastNetworkError       = ENovaNetworkError::Success;
	LastNetworkErrorString = "";
}

/*----------------------------------------------------
    Friends API
----------------------------------------------------*/

void UNovaGameInstance::SetAcceptedInvitationCallback(FOnFriendInviteAccepted Callback)
{
	OnFriendInviteAccepted = Callback;
}

bool UNovaGameInstance::SearchFriends(FOnFriendSearchComplete Callback)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	OnFriendListReady = Callback;

	if (OnlineSub && OnlineSub->GetFriendsInterface())
	{
		ULocalPlayer* Player = GetFirstGamePlayer();

		return OnlineSub->GetFriendsInterface()->ReadFriendsList(
			Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), OnReadFriendsListCompleteDelegate);
	}

	return false;
}

bool UNovaGameInstance::InviteFriend(FUniqueNetIdRepl FriendUserId)
{
	NLOG("UNovaGameInstance::InviteFriend");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Send invite
		return Sessions->SendSessionInviteToFriend(*UserId, GameSessionName, *FriendUserId);
	}

	return false;
}

bool UNovaGameInstance::JoinFriend(FUniqueNetIdRepl FriendUserId)
{
	NLOG("UNovaGameInstance::JoinFriend");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Find the session and join
		ActionAfterError = FNovaSessionAction(GetWorld()->GetName());
		OnFindFriendSessionCompleteDelegateHandle =
			Sessions->AddOnFindFriendSessionCompleteDelegate_Handle(Player->GetControllerId(), OnFindFriendSessionCompleteDelegate);
		return Sessions->FindFriendSession(*UserId, *FriendUserId);
	}

	return false;
}

/*----------------------------------------------------
    Session internals
----------------------------------------------------*/

void UNovaGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNovaGameInstance::OnCreateSessionComplete %s %d", *SessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

	// Clear delegate, start session
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		if (bWasSuccessful)
		{
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);
			Sessions->StartSession(SessionName);
		}
		else
		{
			NetworkState = ENovaNetworkState::Offline;
			OnSessionError(ENovaNetworkError::StartFailed);
		}
	}
}

void UNovaGameInstance::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNovaGameInstance::OnStartOnlineGameComplete %d", bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	// Clear delegate
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
	}

	// Travel to listen server
	if (bWasSuccessful)
	{
		NetworkState = ENovaNetworkState::OnlineHost;

		GetFirstLocalPlayerController()->ClientTravel(NextURL + TEXT("?listen"), ETravelType::TRAVEL_Absolute, false);

		NextURL = FString();
	}
	else
	{
		NetworkState = ENovaNetworkState::Offline;
		OnSessionError(ENovaNetworkError::StartFailed);
	}
}

void UNovaGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNovaGameInstance::OnDestroySessionComplete %d", bWasSuccessful);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);

		if (bWasSuccessful)
		{
			ProcessAction(ActionAfterDestroy);
			return;
		}
	}

	// An error happened
	NetworkState = ENovaNetworkState::Offline;
	OnSessionError(ENovaNetworkError::DestroyFailed);
}

void UNovaGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	NLOG("UNovaGameInstance::OnFindSessionsComplete %d", bWasSuccessful);

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

		EOnlineSessionState::Type SessionState = Sessions->GetSessionState(GameSessionName);
		if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Ended)
		{
			NetworkState = ENovaNetworkState::Offline;
		}
		else
		{
			NetworkState = ENovaNetworkState::OnlineHost;
		}

		OnSessionListReady.ExecuteIfBound(SessionSearch->SearchResults);
	}
}

void UNovaGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	NLOG("UNovaGameInstance::OnJoinSessionComplete");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		// Clear delegate
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

		// Travel to server
		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			NetworkState = ENovaNetworkState::OnlineClient;

			FString TravelURL;
			if (Sessions->GetResolvedConnectString(SessionName, TravelURL))
			{
				GetFirstLocalPlayerController()->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute, false);
			}
		}

		// Handle error
		else
		{
			EOnlineSessionState::Type SessionState = Sessions->GetSessionState(SessionName);
			if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Ended)
			{
				NetworkState = ENovaNetworkState::Offline;
			}
			else
			{
				NetworkState = ENovaNetworkState::OnlineHost;
			}

			switch (Result)
			{
				case EOnJoinSessionCompleteResult::SessionIsFull:
					OnSessionError(ENovaNetworkError::JoinSessionIsFull);
					break;
				case EOnJoinSessionCompleteResult::SessionDoesNotExist:
					OnSessionError(ENovaNetworkError::JoinSessionDoesNotExist);
					break;
				case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
					OnSessionError(ENovaNetworkError::JoinSessionConnectionError);
					break;
				default:
					OnSessionError(ENovaNetworkError::UnknownError);
					break;
			}
		}
	}
}

/*----------------------------------------------------
    Friends internals
----------------------------------------------------*/

void UNovaGameInstance::OnReadFriendsComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	TArray<TSharedRef<FOnlineFriend>> Friends;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub && OnlineSub->GetFriendsInterface().IsValid() && bWasSuccessful)
	{
		ULocalPlayer*     Player   = GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		OnlineSub->GetFriendsInterface()->GetFriendsList(
			Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), Friends);
	}

	OnFriendListReady.ExecuteIfBound(Friends);
}

void UNovaGameInstance::OnSessionUserInviteAccepted(
	bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bWasSuccess)
	{
		OnFriendInviteAccepted.ExecuteIfBound(InviteResult);
	}
}

void UNovaGameInstance::OnFindFriendSessionComplete(
	int32 LocalPlayer, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResult)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindFriendSessionCompleteDelegate_Handle(LocalPlayer, OnFindFriendSessionCompleteDelegateHandle);

		// Join session
		if (bWasSuccessful && SearchResult.Num() > 0 && SearchResult[0].Session.OwningUserId.IsValid() &&
			SearchResult[0].Session.SessionInfo.IsValid())
		{
			JoinSearchResult(SearchResult[0]);
		}

		// Handle error
		else
		{
			EOnlineSessionState::Type SessionState = Sessions->GetSessionState(GameSessionName);
			if (SessionState == EOnlineSessionState::NoSession || SessionState == EOnlineSessionState::Ended)
			{
				NetworkState = ENovaNetworkState::Offline;
			}
			else
			{
				NetworkState = ENovaNetworkState::OnlineHost;
			}

			OnSessionError(ENovaNetworkError::JoinFriendFailed);
		}
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void UNovaGameInstance::OnNetworkError(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	NLOG("UNovaGameInstance::OnNetworkError %d, '%s'", static_cast<int>(FailureType), *ErrorString);

	if (LastNetworkError != ENovaNetworkError::Success)
	{
		return;
	}

	LastNetworkErrorString = ErrorString;

	switch (FailureType)
	{
		case ENetworkFailure::NetDriverAlreadyExists:
		case ENetworkFailure::NetDriverCreateFailure:
		case ENetworkFailure::NetDriverListenFailure:
			LastNetworkError = ENovaNetworkError::NetDriverError;
			break;

		case ENetworkFailure::ConnectionLost:
		case ENetworkFailure::PendingConnectionFailure:
			LastNetworkError = ENovaNetworkError::ConnectionLost;
			break;

		case ENetworkFailure::ConnectionTimeout:
			LastNetworkError = ENovaNetworkError::ConnectionTimeout;
			break;

		case ENetworkFailure::FailureReceived:
			LastNetworkError = ENovaNetworkError::ConnectionFailed;
			break;

		case ENetworkFailure::OutdatedClient:
		case ENetworkFailure::OutdatedServer:
		case ENetworkFailure::NetGuidMismatch:
		case ENetworkFailure::NetChecksumMismatch:
			LastNetworkError = ENovaNetworkError::VersionMismatch;
			break;
	}

	EndSession("");
}

void UNovaGameInstance::OnSessionError(ENovaNetworkError Type)
{
	NLOG("UNovaGameInstance::OnSessionError");

	LastNetworkError       = Type;
	LastNetworkErrorString = "";

	ProcessAction(ActionAfterError);
}

void UNovaGameInstance::ProcessAction(FNovaSessionAction Action)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	NCHECK(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid())
	{
		// We killed the previous session to join a specific one, join it
		if (Action.SessionToJoin.IsValid())
		{
			NetworkState = ENovaNetworkState::Joining;

			ULocalPlayer* Player                = GetFirstGamePlayer();
			OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

			Sessions->JoinSession(*Player->GetPreferredUniqueNetId(), GameSessionName, Action.SessionToJoin);
		}

		// We are playing alone, reload a level
		else if (Action.URL.Len())
		{
			// New online session
			if (Action.Online)
			{
				StartSession(Action.URL, Action.MaxNumPlayers, Action.Public);
			}

			// Exit multiplayer and go back to a level
			else
			{
				NetworkState = ENovaNetworkState::Offline;

				GetFirstLocalPlayerController()->ClientTravel(Action.URL, ETravelType::TRAVEL_Absolute, false);
			}
		}
	}
}

/*----------------------------------------------------
    Getters
----------------------------------------------------*/

FText UNovaGameInstance::GetNetworkStateString() const
{
	switch (NetworkState)
	{
		case ENovaNetworkState::Starting:
			return LOCTEXT("Starting", "Starting");
		case ENovaNetworkState::OnlineHost:
			return LOCTEXT("OnlineHost", "Hosting");
		case ENovaNetworkState::OnlineClient:
			return LOCTEXT("OnlineClient", "Connected");
		case ENovaNetworkState::Searching:
			return LOCTEXT("Searching", "Searching");
		case ENovaNetworkState::Joining:
			return LOCTEXT("Joining", "Joining");
		case ENovaNetworkState::JoiningDestroying:
			return LOCTEXT("JoiningDestroying", "Joining");
		case ENovaNetworkState::Ending:
			return LOCTEXT("Ending", "Ending");
		default:
			return LOCTEXT("Offline", "Offline");
	}
}

FText UNovaGameInstance::GetNetworkErrorString() const
{
	if (!LastNetworkErrorString.IsEmpty())
	{
		return FText::FromString(LastNetworkErrorString);
	}

	switch (LastNetworkError)
	{
		case ENovaNetworkError::Success:
			return LOCTEXT("Success", "Success");
		case ENovaNetworkError::CreateFailed:
			return LOCTEXT("CreateFailed", "Failed to create");
		case ENovaNetworkError::StartFailed:
			return LOCTEXT("StartFailed", "Failed to start session");
		case ENovaNetworkError::DestroyFailed:
			return LOCTEXT("DestroyFailed", "Failed to leave session");
		case ENovaNetworkError::ReadFriendsFailed:
			return LOCTEXT("ReadFriendsFailed", "Couldn't read the friend list");
		case ENovaNetworkError::JoinFriendFailed:
			return LOCTEXT("JoinFriendFailed", "Failed to join friend");
		case ENovaNetworkError::JoinSessionIsFull:
			return LOCTEXT("JoinSessionIsFull", "Session is full");
		case ENovaNetworkError::JoinSessionDoesNotExist:
			return LOCTEXT("JoinSessionDoesNotExist", "Session does not exist anymore");
		case ENovaNetworkError::JoinSessionConnectionError:
			return LOCTEXT("JoinSessionConnectionError", "Failed to join session");

		case ENovaNetworkError::NetDriverError:
			return LOCTEXT("NetDriverError", "Net driver error");
		case ENovaNetworkError::ConnectionLost:
			return LOCTEXT("ConnectionLost", "Connection lost");
		case ENovaNetworkError::ConnectionTimeout:
			return LOCTEXT("ConnectionTimeout", "Connection timed out");
		case ENovaNetworkError::ConnectionFailed:
			return LOCTEXT("ConnectionFailed", "Disconnected");
		case ENovaNetworkError::VersionMismatch:
			return LOCTEXT("VersionMismatch", "Game version mismatch");

		default:
			return LOCTEXT("Unknown", "UnknownError");
	}
}

#undef LOCTEXT_NAMESPACE
