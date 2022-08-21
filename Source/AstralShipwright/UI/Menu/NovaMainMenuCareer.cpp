// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuCareer.h"

#include "NovaMainMenu.h"

#include "Player/NovaPlayerController.h"

#include "Nova.h"

#include "Neutron/Player/NeutronPlayerController.h"
#include "Neutron/System/NeutronMenuManager.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuHome"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuCareer::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot;

	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuCareer::Show()
{
	SNeutronTabPanel::Show();
}

void SNovaMainMenuCareer::Hide()
{
	SNeutronTabPanel::Hide();
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

#undef LOCTEXT_NAMESPACE
