// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaListView.h"

#include "Online.h"


class SNovaMainMenuGame : public SNovaTabPanel
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuGame)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	virtual void AbilityPrimary() override;

	virtual void AbilitySecondary() override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;


protected:

	/** Check if we have a valid friend reference available */
	bool HasSelectedFriend() const;

	/** Get the name of the selected friend */
	FText GetSelectedFriendName() const;

	/** Get the name of a friend */
	FText GetFriendName(TSharedRef<FOnlineFriend> Friend) const;


	/*----------------------------------------------------
		Content callbacks
	----------------------------------------------------*/

protected:

	EVisibility GetContractsTextVisibility() const;
	FText GetContractTitle(uint32 Index) const;
	FText GetContractDescription(uint32 Index) const;
	TOptional<float> GetContractProgress(uint32 Index) const;
	EVisibility GetContractVisibility(uint32 Index) const;
	FText GetContractTrackText(uint32 Index) const;
	const FSlateBrush* GetContractTrackIcon(uint32 Index) const;

	FText GetOnlineText() const;
	FText GetOnlineButtonText() const;
	FText GetOnlineButtonHelpText() const;
	EVisibility GetQuitGameVisibility() const;

	TSharedRef<SWidget> GenerateFriendItem(TSharedRef<FOnlineFriend> Friend);
	const FSlateBrush* GetFriendIcon(TSharedRef<FOnlineFriend> Friend) const;
	FText GenerateFriendTooltip(TSharedRef<FOnlineFriend> Friend);

	FText GetInviteText() const;
	FText GetJoinText() const;
	FText GetOnlineFriendsText() const;
	bool IsInviteFriendEnabled() const;
	bool IsJoinFriendEnabled() const;


	/*----------------------------------------------------
		Callbacks
	----------------------------------------------------*/

	void OnTrackContract(uint32 Index);
	void OnAbandonContract(uint32 Index);

	void OnToggleOnlineGame();
	void OnGoToMainMenu();
	void OnQuitGame();

	void OnFriendListReady(TArray<TSharedRef<FOnlineFriend>> NewFriendList);
	void OnSelectedFriendChanged(TSharedRef<FOnlineFriend> Friend, int32 Index);
	void OnInviteSelectedFriend();
	void OnJoinSelectedFriend();


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Menu manager
	TWeakObjectPtr<UNovaMenuManager>              MenuManager;

	// Widgets
	TSharedPtr<SNovaListView<TSharedRef<FOnlineFriend>>> FriendListView;
	TSharedPtr<SVerticalBox>                      ContractBox;

	// Data
	TArray<TSharedRef<FOnlineFriend>>             FriendList;
	int32                                         SelectedFriendIndex;
	float                                         TimeSinceFriendListUpdate;
	float                                         FriendListUpdatePeriod;

};
