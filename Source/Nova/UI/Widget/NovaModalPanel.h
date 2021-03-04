// Nova project - Gwennaël Arbona

#pragma once

#include "NovaNavigationPanel.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

#include "Widgets/SCompoundWidget.h"

/** Modal panel that blocks input, steals focus and blurs the background */
class SNovaModalPanel : public SNovaNavigationPanel
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaModalPanel) : _Menu(nullptr), _ParentPanel(nullptr)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(SNovaNavigationPanel*, ParentPanel)

	SLATE_ATTRIBUTE(FText, ConfirmText)
	SLATE_ATTRIBUTE(FText, ConfirmHelpText)
	SLATE_ATTRIBUTE(FText, DismissText)
	SLATE_ATTRIBUTE(FText, DismissHelpText)
	SLATE_ATTRIBUTE(FText, CancelText)
	SLATE_ATTRIBUTE(FText, CancelHelpText)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void AbilitySecondary() override;

	virtual bool Confirm() override;

	virtual bool Cancel() override;

	/** Show the panel and take focus, with optional callbacks and an optional content block */
	void Show(FText Title, FText Text, FSimpleDelegate NewOnConfirmed, FSimpleDelegate NewOnIgnore = FSimpleDelegate(),
		FSimpleDelegate NewOnCancel = FSimpleDelegate(), TSharedPtr<SWidget> Content = TSharedPtr<SWidget>());

	/** Hide the panel, set focus back to the parent */
	void Hide();

	/** Check if this panel is visible */
	bool IsVisible() const
	{
		return CurrentAlpha > 0;
	}

	virtual bool IsModal() const override
	{
		return true;
	}

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	EVisibility GetDismissVisibility() const;

	FLinearColor GetColor() const;

	FSlateColor GetBackgroundColor() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/** The panel has been asked by the user to close by confirming */
	void OnConfirmPanel();

	/** The panel has been asked by the user to close without choosing */
	void OnDismissPanel();

	/** The panel has been asked by the user to close by canceling */
	void OnCancelPanel();

	/*----------------------------------------------------
	    Private data
	----------------------------------------------------*/

protected:
	// Parent menu reference
	SNovaNavigationPanel* ParentPanel;

	// Settings
	float           FadeDuration;
	FSimpleDelegate OnConfirmed;
	FSimpleDelegate OnDismissed;
	FSimpleDelegate OnCancelled;

	// Widgets
	TSharedPtr<STextBlock> TitleText;
	TSharedPtr<STextBlock> InformationText;
	TSharedPtr<SBox>       ContentBox;
	TSharedPtr<SWidget>    EmptyWidget;

	// Data
	bool  ShouldShow;
	float CurrentFadeTime;
	float CurrentAlpha;
};
