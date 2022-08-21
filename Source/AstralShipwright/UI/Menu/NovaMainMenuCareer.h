// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronListView.h"

#include "Online.h"

class SNovaMainMenuCareer
	: public SNeutronTabPanel
	, public INeutronGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuCareer)
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

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:

	// Menu manager
	TWeakObjectPtr<class UNeutronMenuManager> MenuManager;
};
