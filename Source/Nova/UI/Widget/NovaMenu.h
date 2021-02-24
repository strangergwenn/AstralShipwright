// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Widgets/SCompoundWidget.h"
#include "Nova/Tools/NovaActorTools.h"

class SNovaMenu : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMenu) : _MenuManager(nullptr)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaMenu();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual bool SupportsKeyboardFocus() const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent) override;

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	/*----------------------------------------------------
	    Input handling
	----------------------------------------------------*/

	/** Reload the key bindings map */
	void UpdateKeyBindings();

	/** Check if a specific action is mapped to that specific key */
	bool IsActionKey(FName ActionName, const FKey& Key) const;

	/** Check if a specific axis is mapped to that specific key */
	bool IsAxisKey(FName AxisName, const FKey& Key) const;

	/*----------------------------------------------------
	    Navigation handling
	----------------------------------------------------*/

	/** Set the currently active navigation panel */
	void SetActiveNavigationPanel(class SNovaNavigationPanel* Panel);

	/** Refresh the current navigation panel */
	void RefreshNavigationPanel();

	/** Unset the currently active navigation panel */
	void ClearNavigationPanel();

	/** Get the currently active navigation panel */
	class SNovaNavigationPanel* GetActiveNavigationPanel() const
	{
		return CurrentNavigationPanel;
	}

	/** Check if a panel is currently used for navigation */
	bool IsActiveNavigationPanel(const SNovaNavigationPanel* Panel) const
	{
		return (CurrentNavigationPanel == Panel);
	}

	/** Create a new modal panel */
	TSharedPtr<class SNovaModalPanel> CreateModalPanel(SNovaNavigationPanel* ParentPanel);

	/** Set FocusButton focused, unfocus others*/
	void SetFocusedButton(TSharedPtr<class SNovaButton> FocusButton, bool FromNavigation);

	/** Get the currently focused button */
	TSharedPtr<SNovaButton> GetFocusedButton();

	/** Get the next button to focus in a specific direction */
	static TSharedPtr<class SNovaButton> GetNextButton(TSharedRef<class SWidget> Widget, TSharedPtr<const class SWidget> Current,
		TArray<TSharedPtr<class SNovaButton>> Candidates, EUINavigation Direction);

	/** Get the next button to focus in a specific direction */
	static TSharedPtr<class SNovaButton> GetNextButtonInternal(TSharedRef<class SWidget> Widget, TSharedPtr<const class SWidget> Current,
		TArray<TSharedPtr<class SNovaButton>> Candidates, EUINavigation Direction);

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	float AnalogNavThreshold;
	float AnalogNavMinPeriod;
	float AnalogNavMaxPeriod;

	// General data
	TWeakObjectPtr<class UNovaMenuManager> MenuManager;
	TSharedPtr<SBox>                       MainContainer;
	TSharedPtr<class SOverlay>             MainOverlay;

	// Mouse input data
	bool      MousePressed;
	bool      MousePressedContinued;
	FVector2D PreviousMousePosition;

	// Input data
	TMultiMap<FName, FName> ActionBindings;
	TMultiMap<FName, FName> AxisBindings;
	FVector2D               CurrentAnalogInput;

	// Focus data
	class SNovaNavigationPanel*           CurrentNavigationPanel;
	TArray<TSharedPtr<class SNovaButton>> CurrentNavigationButtons;
	EUINavigation                         CurrentAnalogNavigation;
	float                                 CurrentAnalogNavigationTime;
};
