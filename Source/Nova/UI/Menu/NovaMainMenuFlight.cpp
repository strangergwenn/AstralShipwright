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
							ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();
							if (IsValid(SpacecraftPawn))
							{
								UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
								NCHECK(PropellantSystem);

								FNumberFormattingOptions Options;
								Options.MaximumFractionalDigits = 0;

								return FText::FormatNamed(LOCTEXT("PropellantFormat", "Propellant left : {remaining} out of {total}"),
									TEXT("remaining"), FText::AsNumber(PropellantSystem->GetAvailablePropellantAmount(), &Options),
									TEXT("total"), FText::AsNumber(PropellantSystem->GetTotalPropellantAmount(), &Options));
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
							ANovaSpacecraftPawn* SpacecraftPawn = MenuManager->GetPC()->GetSpacecraftPawn();
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
								MenuManager->GetPC()->TestJoin();
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

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->TiltInput(Value);
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

ANovaSpacecraftPawn* SNovaMainMenuFlight::GetSpacecraftPawn() const
{
	return MenuManager->GetPC() ? MenuManager->GetPC()->GetSpacecraftPawn() : nullptr;
}

UNovaSpacecraftMovementComponent* SNovaMainMenuFlight::GetSpacecraftMovement() const
{
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	if (IsValid(SpacecraftPawn))
	{
		return Cast<UNovaSpacecraftMovementComponent>(SpacecraftPawn->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
	}
	else
	{
		return nullptr;
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->CanUndock();
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->CanDock();
}

void SNovaMainMenuFlight::OnUndock()
{
	NCHECK(IsUndockEnabled());

	MenuManager->GetPC()->Undock();
}

void SNovaMainMenuFlight::OnDock()
{
	NCHECK(IsDockEnabled());

	MenuManager->GetPC()->Dock();
}

#undef LOCTEXT_NAMESPACE
