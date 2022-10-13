// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronListView.h"

#include "Online.h"

class SNovaMainMenuHome
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuHome)
	{}

	SLATE_ARGUMENT(class SNeutronMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	virtual void Hide() override;

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnLaunchGame(uint32 Index);
	void OnDeleteGame(uint32 Index);
	void OnOpenCredits();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Menu manager
	TWeakObjectPtr<class UNeutronMenuManager> MenuManager;

	// Widgets
	TSharedPtr<class SNeutronModalPanel> ModalPanel;
	TSharedPtr<class SBorder>            CreditsWidget;
};
