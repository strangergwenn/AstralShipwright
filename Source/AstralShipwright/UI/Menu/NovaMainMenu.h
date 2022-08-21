// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/UI/NeutronUI.h"
#include "Neutron/UI/Widgets/NeutronMenu.h"

// Sub-menu types
enum class ENovaMainMenuType : uint8
{
	Home,
	Game,
	Flight,
	Navigation,
	Inventory,
	Assembly,
	Career,
	Settings
};

// Main menu class
class SNovaMainMenu : public SNeutronMenu
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenu)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNeutronMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	SNovaMainMenu()
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	virtual void Hide() override;

	virtual void ShowTooltip(SWidget* TargetWidget, FText Content) override;

	virtual void HideTooltip(SWidget* TargetWidget) override;

	/** Are we on the assembly menu */
	bool IsOnAssemblyMenu() const;

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

	/** Get the credits text visibility */
	EVisibility GetInfoTextVisibility() const;

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
	TSharedPtr<class SNeutronModalPanel> ModalPanel;
	TSharedPtr<class SNeutronTabView>    TabView;
	TSharedPtr<class SNeutronText>       Tooltip;

	// Menus
	TSharedPtr<class SNovaMainMenuHome>       HomeMenu;
	TSharedPtr<class SNovaMainMenuGame>       GameMenu;
	TSharedPtr<class SNovaMainMenuFlight>     FlightMenu;
	TSharedPtr<class SNovaMainMenuNavigation> NavigationMenu;
	TSharedPtr<class SNovaMainMenuOperations> OperationsMenu;
	TSharedPtr<class SNovaMainMenuAssembly>   AssemblyMenu;
	TSharedPtr<class SNovaMainMenuCareer>     CareerMenu;
	TSharedPtr<class SNovaMainMenuSettings>   SettingsMenu;
};
