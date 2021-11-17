// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuHome.h"

#include "NovaMainMenu.h"

#include "Player/NovaPlayerController.h"
#include "System/NovaMenuManager.h"

#include "UI/Component/NovaLargeButton.h"

#include "Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

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
		// Main box
		SNew(SVerticalBox)
		
		+ SVerticalBox::Slot()

		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SImage)
			.Image(FNovaStyleSet::GetBrush("Common/SB_AstralShipwright"))
		]

		+ SVerticalBox::Slot()

		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Bottom)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(Theme.BlurRadius)
			.BlurStrength(Theme.BlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.Padding(Theme.ContentPadding)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()

					+ SHorizontalBox::Slot()
					[
						SNovaDefaultNew(SNovaLargeButton)
						.Theme("MainMenuButton")
						.Icon(FNovaStyleSet::GetBrush("Icon/SB_Menu"))
						.Text(LOCTEXT("Launch1", "Slot 1"))
						.HelpText(LOCTEXT("LaunchHelp1", "Load save data from the first save slot"))
						.OnClicked(FSimpleDelegate::CreateLambda([this]() { OnLaunchGame(1); } ))
					]

					+ SHorizontalBox::Slot()
					[
						SNovaDefaultNew(SNovaLargeButton)
						.Theme("MainMenuButton")
						.Icon(FNovaStyleSet::GetBrush("Icon/SB_Menu"))
						.Text(LOCTEXT("Launch2", "Slot 2"))
						.HelpText(LOCTEXT("LaunchHelp2", "Load save data from the second save slot"))
						.OnClicked(FSimpleDelegate::CreateLambda([this]() { OnLaunchGame(2); } ))
					]

					+ SHorizontalBox::Slot()
					[
						SNovaDefaultNew(SNovaLargeButton)
						.Theme("MainMenuButton")
						.Icon(FNovaStyleSet::GetBrush("Icon/SB_Menu"))
						.Text(LOCTEXT("Launch3", "Slot 3"))
						.HelpText(LOCTEXT("LaunchHelp3", "Load save data from the third save slot"))
						.OnClicked(FSimpleDelegate::CreateLambda([this]() { OnLaunchGame(3); } ))
					]

					+ SHorizontalBox::Slot()
				]
			]
		]
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

void SNovaMainMenuHome::OnLaunchGame(uint32 Index)
{
	MenuManager->GetPC()->StartGame(FString::FromInt(Index), true);
}

#undef LOCTEXT_NAMESPACE
