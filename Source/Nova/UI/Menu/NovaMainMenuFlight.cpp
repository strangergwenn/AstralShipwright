// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/UI/Component/NovaLargeButton.h"

#include "Nova/Nova.h"


#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments()
		.Menu(InArgs._Menu)
	);

	// Structure
	ChildSlot
	[
		SNew(SHorizontalBox)

		// Main box
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Top)
		.AutoWidth()

		+ SHorizontalBox::Slot()
	];
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();

	GetSpacecraftAssembly()->SetHighlightCompartment(INDEX_NONE);
}

void SNovaMainMenuFlight::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (GetSpacecraftAssembly())
	{
		GetSpacecraftAssembly()->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (GetSpacecraftAssembly())
	{
		GetSpacecraftAssembly()->TiltInput(Value);
	}
}


/*----------------------------------------------------
	Internals
----------------------------------------------------*/

ANovaSpacecraftAssembly* SNovaMainMenuFlight::GetSpacecraftAssembly() const
{
	return MenuManager->GetPC()->GetSpacecraftAssembly();
}


/*----------------------------------------------------
	Content callbacks
----------------------------------------------------*/


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/




#undef LOCTEXT_NAMESPACE
