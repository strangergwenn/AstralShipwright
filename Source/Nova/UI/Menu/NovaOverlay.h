// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/NovaUITypes.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Widgets/SCompoundWidget.h"

class SNovaOverlay : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaOverlay)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	/** Show a text notification on the screen */
	void Notify(const FText& Text, ENovaNotificationType Type);

	/** Show a title on the screen */
	void ShowTitle(const FText& Text);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Menu reference
	TWeakObjectPtr<class UNovaMenuManager> MenuManager;

	// Widgets
	TSharedPtr<class SNovaNotification> Notification;
	TSharedPtr<class SNovaTitleCard>    TitleCard;
};
