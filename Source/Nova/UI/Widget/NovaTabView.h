// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "NovaButton.h"
#include "NovaNavigationPanel.h"

#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"

/** Subclass for tab widgets that fade in our out */
class SNovaTabPanel : public SNovaNavigationPanel
{
	friend class SNovaTabView;

public:
	SNovaTabPanel();

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual void OnFocusChanged(TSharedPtr<class SNovaButton> FocusButton) override;

	/** Set the parent tab view */
	void Initialize(int32 Index, bool IsBlurred, TSharedPtr<class SNovaTabView> Parent);

	/** Start showing this menu */
	virtual void Show();

	/** Start hiding this menu */
	virtual void Hide();

	/** Return whether this menu is completely hidden */
	virtual bool IsHidden() const;

	/** Get the current alpha */
	float GetCurrentAlpha() const
	{
		return CurrentAlpha;
	}

	/** Check if this tab should have a blur and darkened background */
	bool IsBlurred() const
	{
		return Blurred;
	}

protected:
	// Data
	bool  Blurred;
	int32 TabIndex;
	bool  CurrentVisible;
	float CurrentAlpha;

	// Widgets
	TSharedPtr<class SScrollBox>   MainContainer;
	TSharedPtr<class SNovaTabView> ParentTabView;
};

/** Tab view class that takes multiple tab slots, and optional toolbar widgets */
class SNovaTabView : public SCompoundWidget
{
public:
	/*----------------------------------------------------
	    Tab slot class
	----------------------------------------------------*/

	class FSlot : public TSlotBase<FSlot>
	{
	public:
		FSlot() : TSlotBase<FSlot>(), Blurred(false), HeaderText(), HeaderHelpText()
		{}

		FSlot& Header(FText Text)
		{
			HeaderText = Text;
			return *this;
		}

		FSlot& HeaderHelp(FText Text)
		{
			HeaderHelpText = Text;
			return *this;
		}

		FSlot& Visible(const TAttribute<bool>& State)
		{
			IsVisible = State;
			return *this;
		}

		FSlot& Visible(const TAttribute<bool>::FGetter& Delegate)
		{
			IsVisible.Bind(Delegate);
			return *this;
		}

		FSlot& Blur()
		{
			Blurred = true;
			return *this;
		}

		FSlot& Expose(FSlot*& OutVarToInit)
		{
			OutVarToInit = this;
			return *this;
		}

		bool             Blurred;
		FText            HeaderText;
		FText            HeaderHelpText;
		TAttribute<bool> IsVisible;
	};

public:
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTabView) : _LeftNavigation(), _RightNavigation(), _End(), _Header()
	{}

	SLATE_SUPPORTS_SLOT(FSlot)

	SLATE_NAMED_SLOT(FArguments, LeftNavigation)

	SLATE_NAMED_SLOT(FArguments, RightNavigation)

	SLATE_NAMED_SLOT(FArguments, End)

	SLATE_NAMED_SLOT(FArguments, Header)

	SLATE_END_ARGS()

public:
	SNovaTabView();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/** Creates a new widget slot */
	static SNovaTabView::FSlot& Slot()
	{
		return *(new SNovaTabView::FSlot());
	}

	/** Re-show the current tab because the game world may have changed */
	void Refresh();

	/** Set the next tab index */
	void ShowPreviousTab();

	/** Set the next tab index */
	void ShowNextTab();

	/** Set the current tab index */
	void SetTabIndex(int32 Index);

	/** Get the current widget slot to activate */
	int32 GetCurrentTabIndex() const;

	/** Get the desired widget slot to activate */
	int32 GetDesiredTabIndex() const;

	/** Get the current alpha */
	float GetCurrentTabAlpha() const;

	/** Check if the tab at a specific index is visible */
	bool IsTabVisible(int32 Index) const;

	/** Get the current widget content root */
	TSharedRef<SNovaTabPanel> GetCurrentTabContent() const;

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/** Check the visibility for a tab */
	EVisibility GetTabVisibility(int32 Index) const;

	/** Check if this tab can be made active */
	bool IsTabEnabled(int32 Index) const;

	/** Get the current color for the entire panel */
	FLinearColor GetColor() const;

	/** Get the current color for the entire panel */
	FSlateColor GetBackgroundColor() const;

	/** Check if we're rendering the split-blur effect or a simple fullscreen one */
	bool IsBlurSplit() const;

	/** Get the padding for the fullscreen blur */
	FMargin GetBlurPadding() const;

	/** Get the current blur radius for the fullscreen blur */
	TOptional<int32> GetBlurRadius() const;

	/** Get the current blur strength for the fullscreen blur */
	float GetBlurStrength() const;

	/** Get the current blur radius for the header-only blur */
	TOptional<int32> GetHeaderBlurRadius() const;

	/** Get the current blur strength for the header-only blur */
	float GetHeaderBlurStrength() const;

protected:
	/*----------------------------------------------------
	    Protected data
	----------------------------------------------------*/

	// Data
	int32                        DesiredTabIndex;
	int32                        CurrentTabIndex;
	TArray<SNovaTabView::FSlot*> SlotInfo;
	float                        CurrentBlurAlpha;

	// Widgets
	TSharedPtr<SBorder>         HeaderContainer;
	TSharedPtr<SHorizontalBox>  Header;
	TSharedPtr<SWidgetSwitcher> Content;
	TArray<SNovaTabPanel*>      Panels;
};
