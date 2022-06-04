// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"

/** Notification widget */
class SNovaNotification : public SNeutronFadingWidget<>
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaNotification)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	void Notify(const FText& Text, const FText& Subtext, ENeutronNotificationType Type);

	virtual bool IsDirty() const override
	{
		return !CurrentNotifyText.EqualTo(DesiredNotifyText) || !CurrentNotifySubtext.EqualTo(DesiredNotifySubtext);
	}

	virtual void OnUpdate() override
	{
		CurrentNotifyText    = DesiredNotifyText;
		CurrentNotifySubtext = DesiredNotifySubtext;
	}

	FText GetNotifyText() const
	{
		return CurrentNotifyText.ToUpper();
	}

	FText GetNotifySubtext() const
	{
		return DesiredNotifySubtext.ToUpper();
	}

	EVisibility GetSubtextVisibility() const
	{
		return CurrentNotifySubtext.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
	}

	FLinearColor GetNotifyColor() const;

	const FSlateBrush* GetNotifyIcon() const;

private:

	// Settings
	TWeakObjectPtr<UNeutronMenuManager> MenuManager;

	// Notification state
	FText                    DesiredNotifyText;
	FText                    DesiredNotifySubtext;
	ENeutronNotificationType DesiredNotifyType;
	FText                    CurrentNotifyText;
	FText                    CurrentNotifySubtext;
};
