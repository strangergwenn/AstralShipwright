// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaListView.h"

#include "Online.h"

class SNovaMainMenuHome
	: public SNovaTabPanel
	, public INovaGameMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuHome)
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

	virtual void Hide() override;

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

	void OnLaunchGame();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Menu manager
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
};
