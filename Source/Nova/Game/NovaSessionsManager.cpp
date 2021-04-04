// Nova project - Gwennaël Arbona

#include "NovaSessionsManager.h"

#include "NovaGameInstance.h"

#include "Nova/UI/NovaUI.h"

#include "Nova/Nova.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "UNovaSessionsManager"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaSessionsManager::UNovaSessionsManager() : Super(), NetworkState(ENovaNetworkState::Offline)
{
	// Session callbacks
	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UNovaSessionsManager::OnCreateSessionComplete);
	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &UNovaSessionsManager::OnStartOnlineGameComplete);
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UNovaSessionsManager::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate  = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UNovaSessionsManager::OnJoinSessionComplete);
	OnDestroySessionCompleteDelegate =
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UNovaSessionsManager::OnDestroySessionComplete);

	// Friends callbacks
	OnReadFriendsListCompleteDelegate = FOnReadFriendsListComplete::CreateUObject(this, &UNovaSessionsManager::OnReadFriendsComplete);
	OnSessionUserInviteAcceptedDelegate =
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UNovaSessionsManager::OnSessionUserInviteAccepted);
	OnFindFriendSessionCompleteDelegate =
		FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &UNovaSessionsManager::OnFindFriendSessionComplete);
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaSessionsManager::Initialize(UNovaGameInstance* Instance)
{
	GameInstance = Instance;

	// Setup network errors
	GameInstance->GetEngine()->OnNetworkFailure().AddUObject(this, &UNovaSessionsManager::OnNetworkError);

	// Setup friends invites
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		OnSessionUserInviteAcceptedDelegateHandle =
			Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
	}
}

void UNovaSessionsManager::Finalize()
{
	// Clean up friends invites
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}
}

/*----------------------------------------------------
    Sessions API
----------------------------------------------------*/

FString UNovaSessionsManager::GetOnlineSubsystemName() const
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		return OnlineSub->GetOnlineServiceName().ToString();
	}

	return FString("Null");
}

bool UNovaSessionsManager::IsOnline() const
{
	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		if (Sessions.IsValid() && UserId.IsValid())
		{
			return Sessions->IsPlayerInSession(GameSessionName, *UserId);
		}
	}

	return false;
}

bool UNovaSessionsManager::IsBusy() const
{
	return (NetworkState != ENovaNetworkState::Offline && NetworkState != ENovaNetworkState::OnlineClient &&
			NetworkState != ENovaNetworkState::OnlineHost);
}

bool UNovaSessionsManager::StartSession(FString URL, int32 MaxNumPlayers, bool Public)
{
	NLOG("UNovaSessionsManager::CreateSession");

	IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
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

bool UNovaSessionsManager::EndSession(FString URL)
{
	NLOG("UNovaSessionsManager::EndSession");

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

void UNovaSessionsManager::SetSessionAdvertised(bool Public)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			FOnlineSessionSettings* Settings = Sessions->GetSessionSettings(GameSessionName);

			if (Settings && Settings->bShouldAdvertise != Public)
			{
				NLOG("UNovaSessionsManager::SetSessionAdvertised %d", Public);

				Settings->bShouldAdvertise = Public;
				Sessions->UpdateSession(GameSessionName, *Settings, true);
			}
		}
	}
}

bool UNovaSessionsManager::SearchSessions(bool OnLan, FOnSessionSearchComplete Callback)
{
	NLOG("UNovaSessionsManager::SearchSessions");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	OnSessionListReady = Callback;

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
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

bool UNovaSessionsManager::JoinSearchResult(const FOnlineSessionSearchResult& SearchResult)
{
	NLOG("UNovaSessionsManager::JoinSearchResult");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
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

void UNovaSessionsManager::ClearErrors()
{
	LastNetworkError       = ENovaNetworkError::Success;
	LastNetworkErrorString = "";
}

/*----------------------------------------------------
    Friends API
----------------------------------------------------*/

void UNovaSessionsManager::SetAcceptedInvitationCallback(FOnFriendInviteAccepted Callback)
{
	OnFriendInviteAccepted = Callback;
}

bool UNovaSessionsManager::SearchFriends(FOnFriendSearchComplete Callback)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	OnFriendListReady = Callback;

	if (OnlineSub && OnlineSub->GetFriendsInterface())
	{
		ULocalPlayer* Player = GameInstance->GetFirstGamePlayer();

		return OnlineSub->GetFriendsInterface()->ReadFriendsList(
			Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), OnReadFriendsListCompleteDelegate);
	}

	return false;
}

bool UNovaSessionsManager::InviteFriend(FUniqueNetIdRepl FriendUserId)
{
	NLOG("UNovaSessionsManager::InviteFriend");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		FUniqueNetIdRepl  UserId   = Player->GetPreferredUniqueNetId();

		// Send invite
		return Sessions->SendSessionInviteToFriend(*UserId, GameSessionName, *FriendUserId);
	}

	return false;
}

bool UNovaSessionsManager::JoinFriend(FUniqueNetIdRepl FriendUserId)
{
	NLOG("UNovaSessionsManager::JoinFriend");

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();

	if (OnlineSub)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
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

void UNovaSessionsManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNovaSessionsManager::OnCreateSessionComplete %s %d", *SessionName.ToString(), bWasSuccessful);

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

void UNovaSessionsManager::OnStartOnlineGameComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNovaSessionsManager::OnStartOnlineGameComplete %d", bWasSuccessful);

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

		GameInstance->GetFirstLocalPlayerController()->ClientTravel(NextURL + TEXT("?listen"), ETravelType::TRAVEL_Absolute, false);

		NextURL = FString();
	}
	else
	{
		NetworkState = ENovaNetworkState::Offline;
		OnSessionError(ENovaNetworkError::StartFailed);
	}
}

void UNovaSessionsManager::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	NLOG("UNovaSessionsManager::OnDestroySessionComplete %d", bWasSuccessful);

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

void UNovaSessionsManager::OnFindSessionsComplete(bool bWasSuccessful)
{
	NLOG("UNovaSessionsManager::OnFindSessionsComplete %d", bWasSuccessful);

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

void UNovaSessionsManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	NLOG("UNovaSessionsManager::OnJoinSessionComplete");

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
				GameInstance->GetFirstLocalPlayerController()->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute, false);
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

void UNovaSessionsManager::OnReadFriendsComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	TArray<TSharedRef<FOnlineFriend>> Friends;

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub && OnlineSub->GetFriendsInterface().IsValid() && bWasSuccessful)
	{
		ULocalPlayer*     Player   = GameInstance->GetFirstGamePlayer();
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		OnlineSub->GetFriendsInterface()->GetFriendsList(
			Player->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), Friends);
	}

	OnFriendListReady.ExecuteIfBound(Friends);
}

void UNovaSessionsManager::OnSessionUserInviteAccepted(
	bool bWasSuccess, const int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bWasSuccess)
	{
		OnFriendInviteAccepted.ExecuteIfBound(InviteResult);
	}
}

void UNovaSessionsManager::OnFindFriendSessionComplete(
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

void UNovaSessionsManager::OnNetworkError(
	UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	NLOG("UNovaSessionsManager::OnNetworkError %d, '%s'", static_cast<int>(FailureType), *ErrorString);

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

void UNovaSessionsManager::OnSessionError(ENovaNetworkError Type)
{
	NLOG("UNovaSessionsManager::OnSessionError");

	LastNetworkError       = Type;
	LastNetworkErrorString = "";

	ProcessAction(ActionAfterError);
}

void UNovaSessionsManager::ProcessAction(FNovaSessionAction Action)
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

			ULocalPlayer* Player                = GameInstance->GetFirstGamePlayer();
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

				GameInstance->GetFirstLocalPlayerController()->ClientTravel(Action.URL, ETravelType::TRAVEL_Absolute, false);
			}
		}
	}
}

/*----------------------------------------------------
    Getters
----------------------------------------------------*/

FText UNovaSessionsManager::GetNetworkStateString() const
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

FText UNovaSessionsManager::GetNetworkErrorString() const
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
