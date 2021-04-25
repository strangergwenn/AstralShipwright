// Nova project - Gwennaël Arbona

#include "NovaMainMenu.h"

#include "NovaMainMenuHome.h"
#include "NovaMainMenuGame.h"
#include "NovaMainMenuFlight.h"
#include "NovaMainMenuNavigation.h"
#include "NovaMainMenuAssembly.h"
#include "NovaMainMenuSettings.h"

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaKeyLabel.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaWindowManipulator.h"
#include "Nova/UI/NovaUITypes.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/System/NovaMenuManager.h"

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
			SNovaNew(SNovaButton)
			.Theme("TabButton")
			.Size("TabButtonSize")
			.Action(FNovaPlayerInput::MenuToggle)
			.Text(this, &SNovaMainMenu::GetCloseText)
			.HelpText(this, &SNovaMainMenu::GetCloseHelpText)
			.OnClicked(this, &SNovaMainMenu::OnClose)
		]

		// Header widget
		.Header()
		[
			SNew(SBorder)
			.Padding(Theme.ContentPadding)
			.BorderImage(new FSlateNoResource)
			.ColorAndOpacity(this, &SNovaMainMenu::GetTooltipColor)
			.Padding(0)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.SubtitleFont)
				.Text(this, &SNovaMainMenu::GetTooltipText)
			]
		]

		// Manipulator
		.BackgroundWidget()
		[
			SNew(SNovaMenuManipulator)
			.Image(&Theme.MainMenuManipulator)
			.ColorAndOpacity(this, &SNovaMainMenu::GetManipulatorColor)
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

		// Navigation menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("NavigationMenuTitle", "Navigation"))
		.HeaderHelp(LOCTEXT("NavigationMenuTitleHelp", "Plot trajectories"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::AreGameMenusVisible))
		.Blur()
		[
			SAssignNew(NavigationMenu, SNovaMainMenuNavigation)
			.Menu(this)
			.MenuManager(MenuManager)
		]

		// Assembly menu
		+ SNovaTabView::Slot()
		.Header(LOCTEXT("AssemblyMenuTitle", "Assembly"))
		.HeaderHelp(LOCTEXT("AssemblyMenuTitleHelp", "Edit spacecraft assembly"))
		.Visible(TAttribute<bool>::FGetter::CreateSP(this, &SNovaMainMenu::IsAssemblyMenuVisible))
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

	ModalPanel = CreateModalPanel();

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

		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn) && SpacecraftPawn->GetCompartmentCount() == 0)
		{
			DesiredMenu = ENovaMainMenuType::Assembly;
		}

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
		if (CurrentNavigationPanel && !CurrentNavigationPanel->IsModal())
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

bool SNovaMainMenu::IsAssemblyMenuVisible() const
{
	if (AreGameMenusVisible())
	{
		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();
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

FText SNovaMainMenu::GetCloseText() const
{
	ANovaPlayerController* PC = MenuManager->GetPC();

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
	ANovaPlayerController* PC = MenuManager->GetPC();

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
	return FText::FromString(CurrentTooltipContent);
}

FLinearColor SNovaMainMenu::GetTooltipColor() const
{
	float Alpha = (CurrentTooltipTime / TooltipFadeDuration);
	Alpha       = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, ENovaUIConstants::EaseStandard);

	FLinearColor Color = FLinearColor::White;
	Color.A *= Alpha;

	return Color;
}

FSlateColor SNovaMainMenu::GetManipulatorColor() const
{
	return MenuManager->GetInterfaceColor();
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
		const ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();

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
			const UNovaGameInstance* GameInstance = MenuManager->GetWorld()->GetGameInstance<UNovaGameInstance>();
			NCHECK(GameInstance);
			double MinutesSinceLastSave = GameInstance->GetMinutesSinceLastSave();

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
