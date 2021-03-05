// Nova project - Gwennaël Arbona

#include "NovaMainMenu.h"

#include "NovaMainMenuHome.h"
#include "NovaMainMenuGame.h"
#include "NovaMainMenuFlight.h"
#include "NovaMainMenuAssembly.h"
#include "NovaMainMenuSettings.h"

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaKeyLabel.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaWindowManipulator.h"
#include "Nova/UI/NovaUITypes.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenu"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenu::SNovaMainMenu()
	: TooltipDelay(0.5f), TooltipFadeDuration(ENovaUIConstants::FadeDurationShort), CurrentTooltipDelay(0), CurrentTooltipTime(0)
{}

void SNovaMainMenu::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Parent constructor
	SNovaMenu::Construct(SNovaMenu::FArguments().MenuManager(InArgs._MenuManager));

	// clang-format off
	MainContainer->SetContent(
		SAssignNew(TabView, SNovaTabView)

		// Navigate left
		.LeftNavigation()
		[
			SNew(SNovaKeyLabel)
			.Key(this, &SNovaMainMenu::GetPreviousTabKey)
		]

		// Navigate right
		.RightNavigation()
		[
			SNew(SNovaKeyLabel)
			.Key(this, &SNovaMainMenu::GetNextTabKey)
		]

		// Close button
		.End()
		[
			SNew(SNovaButton) // No navigation
			.Theme("TabButton")
			.Size("TabButtonSize")
			.Action(FNovaPlayerInput::MenuToggle)
			.Text(this, &SNovaMainMenu::GetCloseText)
			.HelpText(this, &SNovaMainMenu::GetCloseHelpText)
			.OnClicked(this, &SNovaMainMenu::OnClose)
			.Focusable(false)
		]

		// Header widget
		.Header()
		[
			SNew(STextBlock)
			.TextStyle(&Theme.SubtitleFont)
			.Text(this, &SNovaMainMenu::GetTooltipText)
			.ColorAndOpacity(this, &SNovaMainMenu::GetTooltipColor)
		]

		// Manipulator
		.BackgroundWidget()
		[
			SNew(SNovaMenuManipulator)
			.Image(&Theme.MainMenuManipulator)
		]

		// Home menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("HomeMenuTitle", "Home"))
		.HeaderHelp(LOCTEXT("MainMenuTitleHelp", "Start playing"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::IsHomeMenuVisible))
		.Blur()
		[
			SAssignNew(HomeMenu, SNovaMainMenuHome)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Game menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("GameMenuTitle", "Game"))
		.HeaderHelp(LOCTEXT("GameMenuTitleHelp", "Play with your friends"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		.Blur()
		[
			SAssignNew(GameMenu, SNovaMainMenuGame)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Flight menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("FlightMenuTitle", "Flight"))
		.HeaderHelp(LOCTEXT("FlightMenuTitleHelp", "Control your ship"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		[
			SAssignNew(FlightMenu, SNovaMainMenuFlight)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Assembly menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("AssemblyMenuTitle", "Assembly"))
		.HeaderHelp(LOCTEXT("AssemblyMenuTitleHelp", "Edit spacecraft assembly"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		[
			SAssignNew(AssemblyMenu, SNovaMainMenuAssembly)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Settings
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("SettingsMenuTitle", "Settings"))
		.HeaderHelp(LOCTEXT("SettingsMenuTitleHelp", "Setup the game"))
		.Blur()
		[
			SAssignNew(SettingsMenu, SNovaMainMenuSettings)
			.Menu(this)
			.MenuManager(MenuManager)
		]
	);
	// clang-format on

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
		TabView->SetTabIndex(static_cast<uint8>(DesiredMenu));
		WasOnMainMenu = IsOnMenu;
	}

	SetVisibility(EVisibility::Visible);

	TabView->Refresh();
}

void SNovaMainMenu::Hide()
{
	SetVisibility(EVisibility::Hidden);
}

void SNovaMainMenu::ShowTooltip(SWidget* TargetWidget, FText Content)
{
	if (TargetWidget != CurrentTooltipWidget)
	{
		CurrentTooltipWidget  = TargetWidget;
		DesiredTooltipContent = Content.ToString();
	}
}

void SNovaMainMenu::HideTooltip(SWidget* TargetWidget)
{
	if (TargetWidget == CurrentTooltipWidget)
	{
		CurrentTooltipWidget  = nullptr;
		DesiredTooltipContent = FString();
	}

	// If the current focus is valid, and different, show it again
	TSharedPtr<SNovaButton> FocusButton = GetFocusedButton();
	if (FocusButton.IsValid() && FocusButton.Get() != TargetWidget)
	{
		ShowTooltip(FocusButton.Get(), FocusButton->GetHelpText());
	}
}

void SNovaMainMenu::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaMenu::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update tooltip timeline
	if (DesiredTooltipContent != CurrentTooltipContent)
	{
		CurrentTooltipDelay = 0;
	}
	else
	{
		CurrentTooltipDelay += DeltaTime;
	}

	// Update tooltip animation
	if (CurrentTooltipDelay <= TooltipDelay || TabView->GetCurrentTabAlpha() < 1.0f)
	{
		CurrentTooltipTime -= DeltaTime;
	}
	else
	{
		CurrentTooltipTime += DeltaTime;
	}
	CurrentTooltipTime = FMath::Clamp(CurrentTooltipTime, 0.0f, TooltipFadeDuration);

	// Update tooltip content
	if (CurrentTooltipTime == 0)
	{
		CurrentTooltipContent = DesiredTooltipContent;
	}
}

FReply SNovaMainMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	FReply Result = SNovaMenu::OnKeyDown(MyGeometry, KeyEvent);

	if (!Result.IsEventHandled())
	{
		const FKey Key = KeyEvent.GetKey();

		// Move between tabs
		if (!CurrentNavigationPanel->IsModal())
		{
			if (IsActionKey(FNovaPlayerInput::MenuPreviousTab, Key))
			{
				TabView->ShowPreviousTab();
				Result = FReply::Handled();
			}
			else if (IsActionKey(FNovaPlayerInput::MenuNextTab, Key))
			{
				TabView->ShowNextTab();
				Result = FReply::Handled();
			}
		}

		// Exit
		else if (IsActionKey(FNovaPlayerInput::MenuToggle, Key))
		{
			OnClose();
			Result = FReply::Handled();
		}
	}

	return Result;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenu::IsHomeMenuVisible() const
{
	return MenuManager->GetPC() && MenuManager->GetPC()->IsOnMainMenu();
}

bool SNovaMainMenu::AreGameMenusVisible() const
{
	return !IsHomeMenuVisible();
}

FText SNovaMainMenu::GetCloseText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC();

	if (PC && PC->IsOnMainMenu())
	{
		return LOCTEXT("Quit", "Quit game");
	}
	else if (PC && PC->IsMenuOnly())
	{
		return LOCTEXT("Close", "Quit to menu");
	}
	else
	{
		return LOCTEXT("Close", "Close menu");
	}
}

FText SNovaMainMenu::GetCloseHelpText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC();

	if (PC && PC->IsOnMainMenu())
	{
		return LOCTEXT("QuitHelp", "Exit the game");
	}
	else if (PC && PC->IsMenuOnly())
	{
		return LOCTEXT("CloseHelp", "Save and log out of the current game");
	}
	else
	{
		return LOCTEXT("CloseHelp", "Close the main menu");
	}
}

FText SNovaMainMenu::GetTooltipText() const
{
	return FText::FromString(CurrentTooltipContent);
}

FSlateColor SNovaMainMenu::GetTooltipColor() const
{
	float Alpha = (CurrentTooltipTime / TooltipFadeDuration);
	Alpha       = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, ENovaUIConstants::EaseStandard);

	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	FLinearColor          Color = Theme.MainFont.ColorAndOpacity.GetSpecifiedColor();
	Color.A *= Alpha;

	return Color;
}

FKey SNovaMainMenu::GetPreviousTabKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuPreviousTab);
}

FKey SNovaMainMenu::GetNextTabKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuNextTab);
}

/*----------------------------------------------------
    Action callbacks
----------------------------------------------------*/

void SNovaMainMenu::OnClose()
{
	if (MenuManager->GetPC()->IsOnMainMenu())
	{
		MenuManager->GetPC()->ExitGame();
	}
	else if (MenuManager->GetPC()->IsMenuOnly())
	{
		MenuManager->GetPC()->GoToMainMenu();
	}
	else
	{
		MenuManager->GetPC()->ToggleMenuOrQuit();
	}
}

#undef LOCTEXT_NAMESPACE
