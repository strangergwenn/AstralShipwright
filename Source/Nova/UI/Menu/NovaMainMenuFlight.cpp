// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Spacecraft/NovaSpacecraftAssembly.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

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
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaAssignNew(UndockButton, SNovaButton)
				.Text(LOCTEXT("Undock", "Undock"))
				.HelpText(LOCTEXT("UndockHelp", "Undock from the station"))
				.OnClicked(this, &SNovaMainMenuFlight::OnUndock)
				.Enabled(this, &SNovaMainMenuFlight::IsUndockEnabled)
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaAssignNew(DockButton, SNovaButton)
				.Text(LOCTEXT("Dock", "Dock"))
				.HelpText(LOCTEXT("DockHelp", "Dock at the station"))
				.OnClicked(this, &SNovaMainMenuFlight::OnDock)
				.Enabled(this, &SNovaMainMenuFlight::IsDockEnabled)
			]
		]

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

TSharedPtr<SNovaButton> SNovaMainMenuFlight::GetDefaultFocusButton() const
{
	if (IsUndockEnabled())
	{
		return UndockButton;
	}
	else
	{
		return DockButton;
	}
}


/*----------------------------------------------------
	Internals
----------------------------------------------------*/

ANovaSpacecraftAssembly* SNovaMainMenuFlight::GetSpacecraftAssembly() const
{
	return MenuManager->GetPC()->GetSpacecraftAssembly();
}

UNovaSpacecraftMovementComponent* SNovaMainMenuFlight::GetSpacecraftMovement() const
{
	ANovaSpacecraftAssembly* Assembly = GetSpacecraftAssembly();
	NCHECK(Assembly);

	UNovaSpacecraftMovementComponent* MovementComponent = Cast<UNovaSpacecraftMovementComponent>(
		Assembly->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
	
	NCHECK(MovementComponent);

	return MovementComponent;
}


/*----------------------------------------------------
	Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return GetSpacecraftMovement()->GetState() == ENovaMovementState::Docked;
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return GetSpacecraftMovement()->GetState() == ENovaMovementState::Idle;
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

void SNovaMainMenuFlight::OnUndock()
{
	NCHECK(IsUndockEnabled());

	MenuManager->GetPC()->Undock();
}

void SNovaMainMenuFlight::OnDock()
{
	NCHECK(IsDockEnabled());

	MenuManager->GetPC()->Dock(FVector::ZeroVector);
}


#undef LOCTEXT_NAMESPACE
