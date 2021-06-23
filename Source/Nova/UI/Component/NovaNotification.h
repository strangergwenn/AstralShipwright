// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"

/** Notification widget */
class SNovaNotification : public SNovaFadingWidget<>
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaNotification)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	void Notify(const FText& Text, ENovaNotificationType Type);

	virtual bool IsDirty() const
	{
		return !CurrentNotifyText.EqualTo(DesiredNotifyText);
	}

	virtual void OnUpdate() override
	{
		CurrentNotifyText = DesiredNotifyText;
	}

	FText GetNotifyText() const
	{
		return CurrentNotifyText.ToUpper();
	}

	FSlateColor GetNotifyColor() const;

	const FSlateBrush* GetNotifyIcon() const;

private:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;

	// Notification state
	FText                 DesiredNotifyText;
	ENovaNotificationType DesiredNotifyType;
	FText                 CurrentNotifyText;
};
