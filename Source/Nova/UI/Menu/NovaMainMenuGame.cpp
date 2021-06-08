// Nova project - Gwennaël Arbona

#include "NovaMainMenuGame.h"

#include "NovaMainMenu.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Game/NovaGameState.h"

#include "Nova/System/NovaContractManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"
#include "Nova/System/NovaSessionsManager.h"

#include "Nova/UI/Component/NovaLargeButton.h"

#include "Nova/Nova.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuGame"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuGame::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()

		// Friends box
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)

			// Friends title
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(Theme.VerticalContentPadding)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubtitleFont)
				.Text(LOCTEXT("FriendsTitle", "Friends"))
			]
	
			// Friend list info message
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.InfoFont)
				.Text(this, &SNovaMainMenuGame::GetOnlineFriendsText)
			]

			// Friends control
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNovaNew(SNovaButton)
					.Text(this, &SNovaMainMenuGame::GetInviteText)
					.HelpText(LOCTEXT("InviteHelp", "Invite the selected player"))
					.Action(FNovaPlayerInput::MenuPrimary)
					.OnClicked(this, &SNovaMainMenuGame::OnInviteSelectedFriend)
					.Enabled(this, &SNovaMainMenuGame::IsInviteFriendEnabled)
				]
		
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNovaNew(SNovaButton)
					.Text(this, &SNovaMainMenuGame::GetJoinText)
					.HelpText(LOCTEXT("JoinHelp", "Join the selected player"))
					.Action(FNovaPlayerInput::MenuSecondary)
					.OnClicked(this, &SNovaMainMenuGame::OnJoinSelectedFriend)
					.Enabled(this, &SNovaMainMenuGame::IsJoinFriendEnabled)
				]
			]

			// Friend list
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SAssignNew(FriendListView, SNovaListView<TSharedRef<FOnlineFriend>>)
				.Panel(this)
				.ItemsSource(&FriendList)
				.OnGenerateItem(this, &SNovaMainMenuGame::GenerateFriendItem)
				.OnGenerateTooltip(this, &SNovaMainMenuGame::GenerateFriendTooltip)
				.OnSelectionChanged(this, &SNovaMainMenuGame::OnSelectedFriendChanged)
			]
		]

		+ SHorizontalBox::Slot()

		// Main box
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Center)
		.AutoWidth()
		[
			SNew(SVerticalBox)

			// Session title
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(Theme.VerticalContentPadding)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubtitleFont)
				.Text(LOCTEXT("SessionTitle", "Session"))
			]
	
			// Session info message
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.InfoFont)
				.Text(this, &SNovaMainMenuGame::GetOnlineText)
			]
	
			// Session controls
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaDefaultNew(SNovaButton) // Default navigation
					.Size("DoubleButtonSize")
					.Text(this, &SNovaMainMenuGame::GetOnlineButtonText)
					.HelpText(this, &SNovaMainMenuGame::GetOnlineButtonHelpText)
					.OnClicked(this, &SNovaMainMenuGame::OnToggleOnlineGame)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ContractBox, SVerticalBox)

				// Contracts title
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(LOCTEXT("ContractsTitle", "Contracts"))
				]

				// Contracts title
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.InfoFont)
					.Text(LOCTEXT("ContractsNone", "Find contracts in the world to gain units"))
					.Visibility(this, &SNovaMainMenuGame::GetContractsTextVisibility)
				]
			]
		]

		+ SHorizontalBox::Slot()
	];

	// Add contract trackers
	for (uint32 Index = 0; Index < ENovaConstants::MaxContractsCount; Index++)
	{
		ContractBox->AddSlot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SNovaMainMenuGame::GetContractVisibility, Index)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaNew(SNovaButton)
					.Icon(this, &SNovaMainMenuGame::GetContractTrackIcon, Index)
					.OnClicked(this, &SNovaMainMenuGame::OnTrackContract, Index)
					.Text(this, &SNovaMainMenuGame::GetContractTrackText, Index)
					.HelpText(LOCTEXT("ContractTrackHelp", "Track this contract to see its status on the HUD"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNovaNew(SNovaButton)
					.OnClicked(this, &SNovaMainMenuGame::OnAbandonContract, Index)
					.Text(LOCTEXT("ContractAbandon", "Abandon"))
					.HelpText(LOCTEXT("ContractAbandonHelp", "Discard this contract"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(Theme.ContentPadding)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(this, &SNovaMainMenuGame::GetContractTitle, Index)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.MainFont)
					.Text(this, &SNovaMainMenuGame::GetContractDescription, Index)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SProgressBar)
					.Style(&Theme.ProgressBarStyle)
					.Percent(this, &SNovaMainMenuGame::GetContractProgress, Index)
					.BorderPadding(0)
				]
			]
		];
	}
	// clang-format on

	// Defaults
	TimeSinceFriendListUpdate = 0;
	FriendListUpdatePeriod    = 5;
	SelectedFriendIndex       = INDEX_NONE;
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuGame::Show()
{
	SNovaTabPanel::Show();

	TimeSinceFriendListUpdate = FriendListUpdatePeriod;
}

void SNovaMainMenuGame::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Refresh the friend list
	if (!IsHidden())
	{
		UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();

		if (TimeSinceFriendListUpdate > FriendListUpdatePeriod && !SessionsManager->IsBusy())
		{
			SessionsManager->SearchFriends(FOnFriendSearchComplete::CreateRaw(this, &SNovaMainMenuGame::OnFriendListReady));

			TimeSinceFriendListUpdate = 0;
		}

		TimeSinceFriendListUpdate += DeltaTime;
	}
}

bool SNovaMainMenuGame::HasSelectedFriend() const
{
	return SelectedFriendIndex > INDEX_NONE && SelectedFriendIndex < FriendList.Num();
}

FText SNovaMainMenuGame::GetSelectedFriendName() const
{
	if (HasSelectedFriend())
	{
		return GetFriendName(FriendList[SelectedFriendIndex]);
	}
	else
	{
		return FText();
	}
}

FText SNovaMainMenuGame::GetFriendName(TSharedRef<FOnlineFriend> Friend) const
{
	int32   MaxLength  = 50;
	FString FriendName = Friend->GetDisplayName();
	if (FriendName.Len() > MaxLength)
	{
		FriendName = FriendName.LeftChop(MaxLength) + TEXT("...");
	}

	return FText::FromString(FriendName);
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

EVisibility SNovaMainMenuGame::GetContractsTextVisibility() const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	return ContractManager->GetContractCount() == 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SNovaMainMenuGame::GetContractTitle(uint32 Index) const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	if (Index < ContractManager->GetContractCount())
	{
		return ContractManager->GetContractDetails(Index).Title;
	}
	else
	{
		return FText();
	}
}

FText SNovaMainMenuGame::GetContractDescription(uint32 Index) const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	if (Index < ContractManager->GetContractCount())
	{
		return ContractManager->GetContractDetails(Index).Description;
	}
	else
	{
		return FText();
	}
}

TOptional<float> SNovaMainMenuGame::GetContractProgress(uint32 Index) const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	if (Index < ContractManager->GetContractCount())
	{
		return ContractManager->GetContractDetails(Index).Progress;
	}
	else
	{
		return 0;
	}
}

EVisibility SNovaMainMenuGame::GetContractVisibility(uint32 Index) const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	return Index < ContractManager->GetContractCount() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SNovaMainMenuGame::GetContractTrackText(uint32 Index) const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	if (Index != ContractManager->GetTrackedContract())
	{
		return LOCTEXT("ContractTrack", "Track");
	}
	else
	{
		return LOCTEXT("ContractUntrack", "Tracked");
	}
}

const FSlateBrush* SNovaMainMenuGame::GetContractTrackIcon(uint32 Index) const
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	if (Index == ContractManager->GetTrackedContract())
	{
		return FNovaStyleSet::GetBrush("Icon/SB_On");
	}
	else
	{
		return nullptr;
	}
}

FText SNovaMainMenuGame::GetOnlineText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC();
	if (PC == nullptr)
	{
		return FText();
	}

	UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();
	APlayerState*         PlayerState     = PC->PlayerState;

	if (SessionsManager && PlayerState)
	{
		FText StatusMessage;

		// Get the status message
		if (SessionsManager->GetNetworkState() == ENovaNetworkState::Offline)
		{
			StatusMessage = LOCTEXT("PlayingOffline", "You are playing offline");
		}
		else if (SessionsManager->GetNetworkState() == ENovaNetworkState::OnlineHost)
		{
			StatusMessage = LOCTEXT("OnlineHost", "You are playing online as the host");
		}
		else if (SessionsManager->GetNetworkState() == ENovaNetworkState::OnlineClient)
		{
			StatusMessage = FText::FormatNamed(LOCTEXT("OnlineClient", "You are playing online as a guest ({ping}ms ping)"), TEXT("ping"),
				FText::AsNumber(PlayerState->ExactPing));
		}
		else
		{
			StatusMessage = SessionsManager->GetNetworkStateString();
		}

		return StatusMessage;
	}

	return FText();
}

FText SNovaMainMenuGame::GetOnlineButtonText() const
{
	const UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();
	if (SessionsManager->IsOnline())
	{
		return LOCTEXT("GoOffline", "Go offline");
	}
	else
	{
		return LOCTEXT("GoOnline", "Go online");
	}
}

FText SNovaMainMenuGame::GetOnlineButtonHelpText() const
{
	const UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();
	if (SessionsManager->IsOnline())
	{
		return LOCTEXT("GoOfflineHelp", "Terminate the network session and play alone");
	}
	else
	{
		return LOCTEXT("GoOnlineHelp", "Start a new session where you can play with friends");
	}
}

EVisibility SNovaMainMenuGame::GetQuitGameVisibility() const
{
	return (MenuManager->IsOnConsole() ? EVisibility::Collapsed : EVisibility::Visible);
}

TSharedRef<SWidget> SNovaMainMenuGame::GenerateFriendItem(TSharedRef<FOnlineFriend> Friend)
{
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// Get the friend status
	FText FriendStatus;
	switch (Friend->GetPresence().Status.State)
	{
		case EOnlinePresenceState::Online:
			FriendStatus = LOCTEXT("Online", "Online");
			break;
		case EOnlinePresenceState::Chat:
			FriendStatus = LOCTEXT("Chat", "In chat");
			break;
		case EOnlinePresenceState::Away:
			FriendStatus = LOCTEXT("Away", "Away");
			break;
		case EOnlinePresenceState::ExtendedAway:
			FriendStatus = LOCTEXT("ExtendedAway", "Absent");
			break;
		case EOnlinePresenceState::DoNotDisturb:
			FriendStatus = LOCTEXT("DoNotDisturb", "Do not disturb");
			break;
		default:
			FriendStatus = LOCTEXT("Offline", "Offline");
	}

	// Build widget
	return SNew(SHorizontalBox)

		 + SHorizontalBox::Slot()
			   .Padding(ButtonTheme.IconPadding)
			   .AutoWidth()[SNew(SVerticalBox)

							+ SVerticalBox::Slot()
								  .HAlign(HAlign_Left)
								  .VAlign(VAlign_Center)[SNew(SImage).Image(this, &SNovaMainMenuGame::GetFriendIcon, Friend)]]

		 + SHorizontalBox::Slot()
			   .HAlign(HAlign_Center)
			   .VAlign(VAlign_Center)[SNew(STextBlock).TextStyle(&Theme.MainFont).Text(GetFriendName(Friend))]

		 + SHorizontalBox::Slot()
			   .HAlign(HAlign_Center)
			   .VAlign(VAlign_Center)[SNew(STextBlock).TextStyle(&Theme.MainFont).Text(FriendStatus)];
}

const FSlateBrush* SNovaMainMenuGame::GetFriendIcon(TSharedRef<FOnlineFriend> Friend) const
{
	return FriendListView->GetSelectionIcon(Friend);
}

FText SNovaMainMenuGame::GenerateFriendTooltip(TSharedRef<FOnlineFriend> Friend)
{
	return FText::FormatNamed(LOCTEXT("FriendTooltip", "Invite or join {friend} to play together"), TEXT("friend"), GetFriendName(Friend));
}

FText SNovaMainMenuGame::GetOnlineFriendsText() const
{
	const UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();

	// Get useful pointers
	FText InviteText;
	FText JoinText;
	FText CommonText;

	// Build the appropriate message elements
	if (!SessionsManager->IsOnline())
	{
		CommonText = LOCTEXT("FriendOffline", "Go online to join or invite friends");
	}
	else if (HasSelectedFriend())
	{
		FText FriendName = GetSelectedFriendName();

		// Build message elements
		if (!IsInviteFriendEnabled())
		{
			InviteText = FText::FormatNamed(LOCTEXT("FriendNoInvite", "{friend} can't be invited"), TEXT("friend"), FriendName);
		}
		else if (!IsJoinFriendEnabled())
		{
			JoinText = FText::FormatNamed(LOCTEXT("FriendNoJoin", "{friend} can't be joined"), TEXT("friend"), FriendName);
		}
		else
		{
			CommonText =
				FText::FormatNamed(LOCTEXT("FriendTooltip", "Invite or join {friend} to play together"), TEXT("friend"), FriendName);
		}
	}

	// No friend selected or available
	else if (FriendList.Num() == 0)
	{
		CommonText = LOCTEXT("NoFriendsOnline", "No friends are available right now");
	}
	else
	{
		CommonText = LOCTEXT("FriendsOnline", "Select a friend to invite or join");
	}

	// Build the complete message
	if (CommonText.ToString().Len() > 0)
	{
		return CommonText;
	}
	else if (InviteText.ToString().Len() > 0 && JoinText.ToString().Len() == 0)
	{
		return InviteText;
	}
	else if (InviteText.ToString().Len() == 0 && JoinText.ToString().Len() > 0)
	{
		return JoinText;
	}
	else
	{
		return FText::FromString(InviteText.ToString() + TEXT(" - ") + JoinText.ToString());
	}
}

FText SNovaMainMenuGame::GetInviteText() const
{
	if (HasSelectedFriend())
	{
		return FText::FormatNamed(LOCTEXT("InviteFriend", "Invite {friend}"), TEXT("friend"), GetSelectedFriendName());
	}
	else
	{
		return LOCTEXT("Invite", "Invite");
	}
}

FText SNovaMainMenuGame::GetJoinText() const
{
	if (HasSelectedFriend())
	{
		return FText::FormatNamed(LOCTEXT("JoinFriend", "Join {friend}"), TEXT("friend"), GetSelectedFriendName());
	}
	else
	{
		return LOCTEXT("Join", "Join");
	}
}

bool SNovaMainMenuGame::IsInviteFriendEnabled() const
{
	const UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();
	const ANovaGameState*       GameState       = MenuManager->GetWorld()->GetGameState<ANovaGameState>();

	if (GameState && GameState->IsJoinable() && HasSelectedFriend())
	{
		TSharedRef<FOnlineFriend> SelectedFriend = FriendList[SelectedFriendIndex];

		return SessionsManager->IsOnline() && SelectedFriend->GetInviteStatus() != EInviteStatus::Blocked &&
			   SelectedFriend->GetInviteStatus() != EInviteStatus::PendingOutbound;
	}

	return false;
}

bool SNovaMainMenuGame::IsJoinFriendEnabled() const
{
	const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();

	if (GameState && GameState->IsJoinable() && HasSelectedFriend())
	{
		TSharedRef<FOnlineFriend> SelectedFriend = FriendList[SelectedFriendIndex];
		return SelectedFriend->GetPresence().bIsJoinable;
	}

	return false;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuGame::OnTrackContract(uint32 Index)
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	ContractManager->SetTrackedContract(ContractManager->GetTrackedContract() != Index ? Index : INDEX_NONE);
}

void SNovaMainMenuGame::OnAbandonContract(uint32 Index)
{
	UNovaContractManager* ContractManager = UNovaContractManager::Get();
	NCHECK(ContractManager);

	ContractManager->AbandonContract(Index);
}

void SNovaMainMenuGame::OnToggleOnlineGame()
{
	const UNovaSessionsManager* SessionsManager = MenuManager->GetGameInstance()->GetSessionsManager();
	MenuManager->GetPC()->SetGameOnline(!SessionsManager->IsOnline());
}

void SNovaMainMenuGame::OnFriendListReady(TArray<TSharedRef<FOnlineFriend>> NewFriendList)
{
	FriendList.Empty();
	for (auto Friend : NewFriendList)
	{
		// Look for players who are already here
		bool IsHere = false;
		for (const APlayerState* PlayerState : TActorRange<APlayerState>(MenuManager->GetWorld()))
		{
			if (PlayerState->GetUniqueId() == Friend->GetUserId())
			{
				IsHere = true;
			}
		}

		// Add the friend
		FOnlineUserPresence FriendPresence = Friend->GetPresence();
		if (FriendPresence.bIsOnline && !IsHere)
		{
			FriendList.Add(Friend);
		}
	}

	// Refresh the list
	if (FriendList.Num())
	{
		FriendListView->Refresh();
		SlatePrepass(FSlateApplication::Get().GetApplicationScale());
	}
}

void SNovaMainMenuGame::OnSelectedFriendChanged(TSharedRef<FOnlineFriend> Friend, int32 Index)
{
	SelectedFriendIndex = Index;
}

void SNovaMainMenuGame::OnInviteSelectedFriend()
{
	if (HasSelectedFriend())
	{
		NLOG("SNovaMainMenuOnline::OnInviteFriend");
		MenuManager->GetPC()->InviteFriend(FriendList[SelectedFriendIndex]);
	}
}

void SNovaMainMenuGame::OnJoinSelectedFriend()
{
	if (HasSelectedFriend())
	{
		NLOG("SNovaMainMenuOnline::OnJoinFriend");
		MenuManager->GetPC()->JoinFriend(FriendList[SelectedFriendIndex]);
	}
}

#undef LOCTEXT_NAMESPACE
