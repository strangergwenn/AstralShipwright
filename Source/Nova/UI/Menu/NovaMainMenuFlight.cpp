// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuFlight::SNovaMainMenuFlight() : PC(nullptr), SpacecraftPawn(nullptr), SpacecraftMovement(nullptr), OrbitalSimulation(nullptr)
{}

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Bottom)
	[
		SNew(SBackgroundBlur)
		.BlurRadius(this, &SNovaTabPanel::GetBlurRadius)
		.BlurStrength(this, &SNovaTabPanel::GetBlurStrength)
		.bApplyAlphaToBlur(true)
		.Padding(0)
		[
			SNew(SHorizontalBox)

			// Main box
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Fill)
			.AutoWidth()
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.Padding(Theme.ContentPadding)
				[
					SNew(SVerticalBox)
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text_Lambda([this]() -> FText
						{
							if (IsValid(SpacecraftPawn) && IsValid(OrbitalSimulation))
							{
								UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
								NCHECK(PropellantSystem);
								
								float TrajectoryFuelRemaining = OrbitalSimulation->GetPlayerRemainingFuelRequired(SpacecraftPawn->GetSpacecraftIdentifier());

								FNumberFormattingOptions Options;
								Options.MaximumFractionalDigits = 0;

								FText PropellantLine = FText::FormatNamed(LOCTEXT("PropellantFormat", "Propellant : {remaining}T remaining out of {total}T"),
									TEXT("remaining"), FText::AsNumber(PropellantSystem->GetCurrentPropellantAmount(), &Options),
									TEXT("total"), FText::AsNumber(PropellantSystem->GetTotalPropellantAmount(), &Options));
								
								FText TrajectoryLine;
								if (TrajectoryFuelRemaining > 0)
								{
									TrajectoryLine = FText::FormatNamed(LOCTEXT("TrajectoryFormat", "Trajectory : {remaining}T of propellant still needed"),
										TEXT("remaining"), FText::AsNumber(TrajectoryFuelRemaining, &Options));

									return FText::FromString(PropellantLine.ToString() + "\n" + TrajectoryLine.ToString());
								}

								return PropellantLine;
							}

							return FText();
						})
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("Refill", "Refill spacecraft"))
						.HelpText(LOCTEXT("RefillHelp", "JRefill the spacecraft propellant"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							if (IsValid(SpacecraftPawn))
							{
								UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
								NCHECK(PropellantSystem);
								PropellantSystem->Refill();
							}
						}))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TestJoin", "Join random session"))
						.HelpText(LOCTEXT("TestJoinHelp", "Join random session"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							#if WITH_EDITOR
								PC->TestJoin();
							#endif
						}))
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
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaAssignNew(UndockButton, SNovaButton)
						.Text(LOCTEXT("Undock", "Undock"))
						.HelpText(LOCTEXT("UndockHelp", "Undock from the station"))
						.OnClicked(this, &SNovaMainMenuFlight::OnUndock)
						.Enabled(this, &SNovaMainMenuFlight::IsUndockEnabled)
					]
				]
			]
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);
}

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();
}

void SNovaMainMenuFlight::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuFlight::UpdateGameObjects()
{
	PC                 = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	SpacecraftPawn     = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	SpacecraftMovement = IsValid(SpacecraftPawn) ? SpacecraftPawn->GetSpacecraftMovement() : nullptr;
	OrbitalSimulation  = MenuManager.IsValid() ? UNovaOrbitalSimulationComponent::Get(MenuManager.Get()) : nullptr;
}

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->TiltInput(Value);
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
    Callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return IsValid(PC) && IsValid(SpacecraftPawn) && !SpacecraftPawn->HasModifications() && SpacecraftPawn->IsSpacecraftValid() &&
		   SpacecraftMovement->CanUndock();
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return IsValid(PC) && IsValid(SpacecraftMovement) && SpacecraftMovement->CanDock();
}

void SNovaMainMenuFlight::OnUndock()
{
	if (IsUndockEnabled())
	{
		PC->Undock();
	}
}

void SNovaMainMenuFlight::OnDock()
{
	if (IsDockEnabled())
	{
		PC->Dock();
	}
}

#undef LOCTEXT_NAMESPACE
