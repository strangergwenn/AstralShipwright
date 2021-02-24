// Nova project - Gwennaël Arbona

#include "NovaMainMenuHome.h"

#include "NovaMainMenu.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/UI/Component/NovaLargeButton.h"

#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuHome"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuHome::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SNew(SHorizontalBox)

		// Main box
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Top)
		.AutoWidth()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			[
				SNovaDefaultNew(SNovaLargeButton)
				.Icon(FNovaStyleSet::GetBrush("Icon/SB_Menu"))
				.Text(LOCTEXT("Launch", "Launch game"))
				.HelpText(LOCTEXT("LaunchHelp", "Start a new session"))
				.OnClicked(this, &SNovaMainMenuHome::OnLaunchGame)
			]
		]

		+ SHorizontalBox::Slot()
	];
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuHome::Show()
{
	SNovaTabPanel::Show();
}

void SNovaMainMenuHome::Hide()
{
	SNovaTabPanel::Hide();
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuHome::OnLaunchGame()
{
	MenuManager->GetPC()->StartGame("1", true);
}

#undef LOCTEXT_NAMESPACE
