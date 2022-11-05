// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenu.h"

#include "NovaMainMenuHome.h"
#include "NovaMainMenuFlight.h"
#include "NovaMainMenuNavigation.h"
#include "NovaMainMenuOperations.h"
#include "NovaMainMenuAssembly.h"
#include "NovaMainMenuCareer.h"
#include "NovaMainMenuSettings.h"

#include "Game/NovaGameState.h"
#include "Player/NovaPlayerController.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova.h"

#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/System/NeutronSaveManager.h"
#include "Neutron/UI/Widgets/NeutronButton.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"
#include "Neutron/UI/Widgets/NeutronKeyLabel.h"
#include "Neutron/UI/Widgets/NeutronTabView.h"
#include "Neutron/UI/Widgets/NeutronWindowManipulator.h"
#include "Neutron/UI/NeutronUI.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenu"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenu::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	InArgs._MenuManager->SetInterfaceColor(FLinearColor::White, Theme.PositiveColor);

	// Parent constructor
	SNeutronMenu::Construct(SNeutronMenu::FArguments().MenuManager(InArgs._MenuManager));

	// clang-format off
	MainContainer->SetContent(
		SAssignNew(TabView, SNeutronTabView)

		// Navigate left
		.LeftNavigation()
		[
			SNew(SNeutronKeyLabel)
			.Action(FNeutronPlayerInput::MenuPreviousTab)
		]

		// Navigate right
		.RightNavigation()
		[
			SNew(SNeutronKeyLabel)
			.Action(FNeutronPlayerInput::MenuNextTab)
		]

		// Close button
		.End()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNeutronNew(SNeutronButton)
				.Theme("TabButton")
				.Size("SmallButtonSize")
				.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Maximize"))
				.HelpText(LOCTEXT("MaximizeRestore", "Maximize or restore the window"))
				.OnClicked(this, &SNovaMainMenu::OnMaximizeRestore)
				.Visibility(this, &SNovaMainMenu::GetMaximizeVisibility)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNeutronNew(SNeutronButton)
				.Theme("TabButton")
				.Size("TabButtonSize")
				.Action(FNeutronPlayerInput::MenuToggle)
				.Text(this, &SNovaMainMenu::GetCloseText)
				.HelpText(this, &SNovaMainMenu::GetCloseHelpText)
				.OnClicked(this, &SNovaMainMenu::OnClose)
			]
		]

		// Header widget
		.Header()
		[
			SNew(SBorder)
			.Padding(Theme.ContentPadding)
			.BorderImage(&Theme.MainMenuGenericBorder)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SAssignNew(Tooltip, SNeutronRichText)
					.TextStyle(&Theme.MainFont)
					.Text(FNeutronTextGetter::CreateSP(this, &SNovaMainMenu::GetTooltipText))
				]
		
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.TextStyle(&Theme.MainFont)
					.Text(this, &SNovaMainMenu::GetInfoText)
					.Visibility(this, &SNovaMainMenu::GetInfoTextVisibility)
				]
			]
		]

		// Manipulator
		.BackgroundWidget()
		[
			SNew(SNeutronMenuManipulator)
			.Image(&Theme.MainMenuManipulator)
			.ColorAndOpacity(this, &SNovaMainMenu::GetManipulatorColor)
		]

		// Home menu
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("HomeMenuTitle", "Home"))
		.HeaderHelp(LOCTEXT("MainMenuTitleHelp", "Start playing"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Home"))
		.Visible(this, &SNovaMainMenu::IsHomeMenuVisible)
		[
			SAssignNew(HomeMenu, SNovaMainMenuHome)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Flight menu
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("FlightMenuTitle", "Flight"))
		.HeaderHelp(LOCTEXT("FlightMenuTitleHelp", "Control your ship"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Flight"))
		.Visible(this, &SNovaMainMenu::AreGameMenusVisible)
		.Blur(false)
		[
			SAssignNew(FlightMenu, SNovaMainMenuFlight)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Navigation menu
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("NavigationMenuTitle", "Navigation"))
		.HeaderHelp(LOCTEXT("NavigationMenuTitleHelp", "Plot trajectories"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Navigation"))
		.Visible(this, &SNovaMainMenu::AreGameMenusVisible)
		.Background(FNeutronStyleSet::GetBrush("Map/SB_Background"))
		.Blur(true)
		[
			SAssignNew(NavigationMenu, SNovaMainMenuNavigation)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Operations menu
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("OperationsMenuTitle", "Operations"))
		.HeaderHelp(LOCTEXT("OperationsMenuTitleHelp", "Manage your spacecraft's cargo and systems"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Inventory"))
		.Blur(true)
		.Visible(this, &SNovaMainMenu::AreGameMenusVisible)
		[
			SAssignNew(OperationsMenu, SNovaMainMenuOperations)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Assembly menu
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("AssemblyMenuTitle", "Assembly"))
		.HeaderHelp(LOCTEXT("AssemblyMenuTitleHelp", "Edit spacecraft assembly"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Assembly"))
		.Visible(this, &SNovaMainMenu::IsAssemblyMenuVisible)
		.Blur(false)
		[
			SAssignNew(AssemblyMenu, SNovaMainMenuAssembly)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Assembly menu
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("CareerMenuTitle", "Career"))
		.HeaderHelp(LOCTEXT("CareerMenuTitleHelp", "Unlock new spacecraft parts"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Career"))
		.Visible(this, &SNovaMainMenu::AreGameMenusVisible)
		.Blur(true)
		[
			SAssignNew(CareerMenu, SNovaMainMenuCareer)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Settings
		+ SNeutronTabView::Slot()
		.Header(LOCTEXT("SettingsMenuTitle", "Settings"))
		.HeaderHelp(LOCTEXT("SettingsMenuTitleHelp", "Setup the game"))
		.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Settings"))
		.Blur(true)
		[
			SAssignNew(SettingsMenu, SNovaMainMenuSettings)
			.Menu(this)
			.MenuManager(MenuManager)
		]
	);
	// clang-format on

	// Create ourselves a modal panel
	ModalPanel = CreateModalPanel();

	// Register menus
	MenuManager->RegisterGameMenu(HomeMenu);
	MenuManager->RegisterGameMenu(FlightMenu);
	MenuManager->RegisterGameMenu(NavigationMenu);
	MenuManager->RegisterGameMenu(OperationsMenu);
	MenuManager->RegisterGameMenu(AssemblyMenu);
	MenuManager->RegisterGameMenu(CareerMenu);
	MenuManager->RegisterGameMenu(SettingsMenu);

	// Set the default menu
	WasOnMainMenu = true;
	TabView->SetTabIndex(static_cast<uint8>(ENovaMainMenuType::Home));
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenu::Show()
{
	// Set the default menu
	bool IsOnMenu = MenuManager->GetPC()->IsOnMainMenu();
	if (IsOnMenu != WasOnMainMenu)
	{
		ENovaMainMenuType DesiredMenu = IsOnMenu ? ENovaMainMenuType::Home : ENovaMainMenuType::Flight;

		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC<ANovaPlayerController>()->GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn) && SpacecraftPawn->GetCompartmentCount() == 0)
		{
			DesiredMenu = ENovaMainMenuType::Assembly;
		}

		TabView->SetTabIndex(static_cast<uint8>(DesiredMenu));
		WasOnMainMenu = IsOnMenu;
	}

	SetVisibility(EVisibility::Visible);
}

void SNovaMainMenu::Hide()
{
	SetVisibility(EVisibility::Hidden);
}

void SNovaMainMenu::ShowTooltip(SWidget* TargetWidget, FText Content)
{
	if (TargetWidget != CurrentTooltipWidget)
	{
		CurrentTooltipWidget = TargetWidget;
		CurrentTooltipText   = Content;
	}
}

void SNovaMainMenu::HideTooltip(SWidget* TargetWidget)
{
	if (TargetWidget == CurrentTooltipWidget)
	{
		CurrentTooltipWidget = nullptr;
		CurrentTooltipText   = FText();
	}

	// If the current focus is valid, and different, show it again
	TSharedPtr<SNeutronButton> FocusButton = GetFocusedButton();
	if (FocusButton.IsValid() && FocusButton.Get() != TargetWidget)
	{
		ShowTooltip(FocusButton.Get(), FocusButton->GetHelpText());
	}
}

bool SNovaMainMenu::IsOnAssemblyMenu() const
{
	return TabView->GetCurrentTabIndex() == static_cast<int32>(ENovaMainMenuType::Assembly);
}

FReply SNovaMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	FReply Result = SNeutronMenu::OnKeyDown(MyGeometry, KeyEvent);

	if (!Result.IsEventHandled())
	{
		const FKey Key = KeyEvent.GetKey();

		// Move between tabs
		if (CurrentNavigationPanel && !CurrentNavigationPanel->IsModal())
		{
			if (IsActionKey(FNeutronPlayerInput::MenuPreviousTab, Key))
			{
				TabView->ShowPreviousTab();
				Result = FReply::Handled();
			}
			else if (IsActionKey(FNeutronPlayerInput::MenuNextTab, Key))
			{
				TabView->ShowNextTab();
				Result = FReply::Handled();
			}
		}
	}

	return Result;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenu::IsHomeMenuVisible() const
{
	return MenuManager.IsValid() && IsValid(MenuManager->GetPC()) && MenuManager->GetPC()->IsOnMainMenu();
}

bool SNovaMainMenu::IsAssemblyMenuVisible() const
{
	if (AreGameMenusVisible() && IsValid(MenuManager->GetPC()))
	{
		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC<ANovaPlayerController>()->GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn))
		{
			return SpacecraftPawn->IsDocked();
		}
	}

	return false;
}

bool SNovaMainMenu::AreGameMenusVisible() const
{
	return !IsHomeMenuVisible();
}

EVisibility SNovaMainMenu::GetInfoTextVisibility() const
{
	return AreGameMenusVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SNovaMainMenu::GetMaximizeVisibility() const
{
	return GEngine->GetGameUserSettings()->GetFullscreenMode() == EWindowMode::Windowed ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SNovaMainMenu::GetCloseText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC<ANovaPlayerController>();

	if (PC && PC->IsOnMainMenu())
	{
		return LOCTEXT("QuitGame", "Quit game");
	}
	else if (PC && PC->IsMenuOnly())
	{
		return LOCTEXT("QuitMenu", "Quit to menu");
	}
	else
	{
		return LOCTEXT("Close", "Close menu");
	}
}

FText SNovaMainMenu::GetCloseHelpText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC<ANovaPlayerController>();

	if (PC && PC->IsOnMainMenu())
	{
		return LOCTEXT("QuitGameHelp", "Exit the game");
	}
	else if (PC && PC->IsMenuOnly())
	{
		return LOCTEXT("QuitMenuHelp", "Save and log out of the current game");
	}
	else
	{
		return LOCTEXT("CloseHelp", "Close the main menu");
	}
}

FText SNovaMainMenu::GetTooltipText() const
{
	FString TooltipText = CurrentTooltipText.ToString();
	return TooltipText.Len() > 0 ? FText::FromString(TEXT("<img src=\"/Text/Info\"/> ") + TooltipText) : FText();
}

FText SNovaMainMenu::GetInfoText() const
{
	const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();

	if (IsValid(GameState) && IsValid(MenuManager->GetPC()))
	{
		FNovaCredits PlayerCredits = MenuManager->GetPC<ANovaPlayerController>()->GetAccountBalance();

		return FText::FormatNamed(LOCTEXT("InfoText", "{date} - {credits} in account"), TEXT("date"),
			::GetDateText(GameState->GetCurrentTime()), TEXT("credits"), GetPriceText(PlayerCredits));
	}

	return FText();
}

FSlateColor SNovaMainMenu::GetManipulatorColor() const
{
	return MenuManager->GetInterfaceColor();
}

/*----------------------------------------------------
    Action callbacks
----------------------------------------------------*/

void SNovaMainMenu::OnMaximizeRestore()
{
	MenuManager->MaximizeOrRestore();
}

void SNovaMainMenu::OnClose()
{
	if (MenuManager->GetPC()->IsOnMainMenu())
	{
		MenuManager->GetPC()->ExitGame();
	}
	else if (MenuManager->GetPC()->IsMenuOnly())
	{
		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC<ANovaPlayerController>()->GetSpacecraftPawn();

		// Ship is docked, we will save and quit
		if (IsValid(SpacecraftPawn) && SpacecraftPawn->IsDocked())
		{
			ModalPanel->Show(LOCTEXT("ConfirmQuit", "Quit to menu ?"), LOCTEXT("ConfirmQuitHelp", "Your progression will be saved."),
				FSimpleDelegate::CreateLambda(
					[&]()
					{
						MenuManager->GetPC()->GoToMainMenu(true);
					}));
		}

		// Ship is not docked, some progress will be lost
		else
		{
			double MinutesSinceLastSave = UNeutronSaveManager::Get()->GetMinutesSinceLastSave();

			ModalPanel->Show(LOCTEXT("ConfirmQuitWithoutSaving", "Quit without saving ?"),
				FText::FormatNamed(LOCTEXT("ConfirmQuitWithoutSavingHelp",
									   "You will lose progress since the last save, {minutes} {minutes}|plural(one=minute,other=minutes) "
									   "ago. Dock at a station to save the game."),
					TEXT("minutes"), FText::AsNumber(FMath::CeilToInt(MinutesSinceLastSave))),
				FSimpleDelegate::CreateLambda(
					[&]()
					{
						MenuManager->GetPC()->GoToMainMenu(false);
					}));
		}
	}
	else
	{
		MenuManager->GetPC()->ToggleMenuOrQuit();
	}
}

#undef LOCTEXT_NAMESPACE
