// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"

#include "Widgets/Input/SButton.h"


/** Animation state used by lists to seamlessly animate on refresh */
struct FNovaButtonState
{
	FNovaButtonState()
		: CurrentColorAnimationAlpha(0)
		, CurrentSizeAnimationAlpha(0)
		, CurrentDisabledAnimationAlpha(0)
		, CurrentTimeSinceClicked(10000)
	{}

	float CurrentColorAnimationAlpha;
	float CurrentSizeAnimationAlpha;
	float CurrentDisabledAnimationAlpha;

	float CurrentTimeSinceClicked;
};


/** Base button class */
class SNovaButton : public SButton
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaButton)
		: _Theme("DefaultButton")
		, _Size("DefaultButtonSize")
		, _BorderRotation(0)
		, _Enabled(true)
		, _Focusable(true)
		, _Toggle(false)
		, _Header()
		, _Footer()
	{}

	SLATE_ATTRIBUTE(FText, Text)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)
	SLATE_ARGUMENT(float, BorderRotation)

	SLATE_ATTRIBUTE(bool, Enabled)
	SLATE_ATTRIBUTE(bool, Focusable)
	SLATE_ARGUMENT(bool, Toggle)
	
	SLATE_NAMED_SLOT(FArguments, Header)
	SLATE_NAMED_SLOT(FArguments, Footer)

	SLATE_EVENT(FSimpleDelegate, OnFocused)
	SLATE_EVENT(FSimpleDelegate, OnClicked)
	SLATE_EVENT(FSimpleDelegate, OnDoubleClicked)
	
	SLATE_END_ARGS()


public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Public methods
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	bool SupportsKeyboardFocus() const override
	{
		return ButtonFocusable.Get(true) && IsButtonEnabled();
	}

	/** Mouse click	*/
	virtual FReply OnButtonClicked();

	/** Mouse double click */
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** Set the focused state */
	void SetFocused(bool State);

	/** Get the focused state */
	bool IsFocused() const;

	/** Check if this button is locked */
	bool IsButtonEnabled() const;

	/** Set the active state */
	void SetActive(bool State);

	/** Get the active state */
	bool IsActive() const;

	/** Force a new text */
	void SetText(FText NewText);

	/** Force a new help text */
	void SetHelpText(FText NewText);

	/** Get the current help text */
	FText GetHelpText();

	/** Get the internal state */
	FNovaButtonState& GetState()
	{
		return State;
	}


	/*----------------------------------------------------
		Callbacks
	----------------------------------------------------*/

protected:

	/** Get the key binding for closing this menu */
	FKey GetActionKey() const;

	/** Icon brush callback */
	const FSlateBrush* GetIconBrush() const;

	/** Activity overlay color */
	FSlateColor GetBackgroundBrushColor() const;

	/** Overall button color */
	FLinearColor GetMainColor() const;

	/** Current width callback */
	FOptionalSize GetWidth() const;

	/** Current height callback */
	FOptionalSize GetHeight() const;

	/** Get the text font */
	FSlateFontInfo GetFont() const;

	/** Get the render transform of the border */
	TOptional<FSlateRenderTransform> GetBorderRenderTransform() const;


protected:

	/*----------------------------------------------------
		Private data
	----------------------------------------------------*/
	
	// Settings & attributes
	TAttribute<bool>                              ButtonEnabled;
	TAttribute<bool>                              ButtonFocusable;
	TAttribute<FText>                             Text;
	TAttribute<FText>                             HelpText;
	TAttribute<FName>                             Action;
	TAttribute<const struct FSlateBrush*>         Icon;
	FName                                         ThemeName;
	FName                                         SizeName;
	float                                         BorderRotation;
	bool                                          IsToggle;
	float                                         AnimationDuration;
	FSimpleDelegate                               OnFocused;
	FSimpleDelegate                               OnClicked;
	FSimpleDelegate                               OnDoubleClicked;

	// State
	bool                                          Focused;
	bool                                          Hovered;
	bool                                          Active;
	FNovaButtonState                              State;
	
	// Slate elements
	TSharedPtr<class SBorder>                     InnerContainer;
	TSharedPtr<class STextBlock>                  TextBlock;


public:

	/*----------------------------------------------------
		Get & Set
	----------------------------------------------------*/

	TSharedPtr<SBorder> GetContainer() const
	{
		return InnerContainer;
	}
};
