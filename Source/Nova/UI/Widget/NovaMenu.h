// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Widgets/SCompoundWidget.h"

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

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

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
	void SetNavigationPanel(class SNovaNavigationPanel* Panel);

	/** Unset the currently active navigation panel */
	void ClearNavigationPanel();

	/** Refresh the current navigation panel */
	void RefreshNavigationPanel();

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
	template <typename T = class SNovaModalPanel>
	TSharedPtr<T> CreateModalPanel()
	{
		TSharedPtr<T> Panel;

		MainOverlay->AddSlot()[SAssignNew(Panel, T).Menu(this)];

		return Panel;
	}

	/** Set a currently active navigation panel */
	void SetModalNavigationPanel(class SNovaNavigationPanel* Panel);

	/** Set a currently active navigation panel */
	void ClearModalNavigationPanel();

	/** Set FocusButton focused, unfocus others*/
	void SetFocusedButton(TSharedPtr<class SNovaButton> FocusButton, bool FromNavigation);

	/** Get the currently focused button */
	TSharedPtr<SNovaButton> GetFocusedButton();

protected:
	/** Create a new button that can be triggered by actions */
	template <typename WidgetType, typename RequiredArgsPayloadType>
	TDecl<WidgetType, RequiredArgsPayloadType> NewNovaButton(
		const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine, RequiredArgsPayloadType&& InRequiredArgs, bool DefaultFocus)
	{
		auto Button = TDecl<WidgetType, RequiredArgsPayloadType>(InType, InFile, OnLine, Forward<RequiredArgsPayloadType>(InRequiredArgs));

		AdditionalActionButtons.Add(Button._Widget);

		return Button;
	}

	/** Get all action buttons */
	TArray<TSharedPtr<class SNovaButton>> GetActionButtons() const
	{
		TArray<TSharedPtr<class SNovaButton>> Result = CurrentNavigationButtons;
		Result.Append(AdditionalActionButtons);
		return Result;
	}

	/** Trigger actions bounds to this key */
	FReply HandleKeyPress(FKey Key);

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
	class SNovaNavigationPanel*           PreviousNavigationPanel;
	TArray<TSharedPtr<class SNovaButton>> CurrentNavigationButtons;
	TArray<TSharedPtr<class SNovaButton>> AdditionalActionButtons;
	EUINavigation                         CurrentAnalogNavigation;
	float                                 CurrentAnalogNavigationTime;
};
