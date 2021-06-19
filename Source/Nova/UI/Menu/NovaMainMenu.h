// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaMenu.h"

// Sub-menu types
enum class ENovaMainMenuType : uint8
{
	Home,
	Game,
	Flight,
	Navigation,
	Inventory,
	Assembly,
	Settings
};

// Main menu class
class SNovaMainMenu : public SNovaMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenu)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaMainMenu()
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	/** Show this menu */
	void Show();

	/** Start hiding this menu */
	void Hide();

	/** Start displaying the tooltip */
	void ShowTooltip(SWidget* TargetWidget, FText Content);

	/** Stop displaying the tooltip */
	void HideTooltip(SWidget* TargetWidget);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;

	/*----------------------------------------------------
	    Content callbacks
	----------------------------------------------------*/

protected:
	/** Check if we can display the home menu */
	bool IsHomeMenuVisible() const;

	/** Check if we can display the assembly menu */
	bool IsAssemblyMenuVisible() const;

	/** Check if we can display the game menus */
	bool AreGameMenusVisible() const;

	/** Get the visibility of the window control */
	EVisibility GetMaximizeVisibility() const;

	/** Get the close button text */
	FText GetCloseText() const;

	/** Get the close button help text */
	FText GetCloseHelpText() const;

	/** Get the title text */
	FText GetTooltipText() const;

	/** Get the info text */
	FText GetInfoText() const;

	/** Get the manipulator color */
	FSlateColor GetManipulatorColor() const;

	/** Get the key binding for the previous tab */
	FKey GetPreviousTabKey() const;

	/** Get the key binding for the next tab */
	FKey GetNextTabKey() const;

	/*----------------------------------------------------
	    Action callbacks
	----------------------------------------------------*/

protected:
	/** Maximize or restore the window */
	void OnMaximizeRestore();

	/** Quit the game */
	void OnClose();

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// General state
	SWidget* CurrentTooltipWidget;
	FText    CurrentTooltipText;
	bool     WasOnMainMenu;

	// Widgets
	TSharedPtr<class SNovaModalPanel> ModalPanel;
	TSharedPtr<class SNovaTabView>    TabView;
	TSharedPtr<class SNovaText>       Tooltip;

	// Menus
	TSharedPtr<class SNovaMainMenuHome>       HomeMenu;
	TSharedPtr<class SNovaMainMenuGame>       GameMenu;
	TSharedPtr<class SNovaMainMenuFlight>     FlightMenu;
	TSharedPtr<class SNovaMainMenuNavigation> NavigationMenu;
	TSharedPtr<class SNovaMainMenuInventory>  InventoryMenu;
	TSharedPtr<class SNovaMainMenuAssembly>   AssemblyMenu;
	TSharedPtr<class SNovaMainMenuSettings>   SettingsMenu;
};
