// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuFlight::SNovaMainMenuFlight()
	: PC(nullptr), SpacecraftPawn(nullptr), SpacecraftMovement(nullptr), GameState(nullptr), OrbitalSimulation(nullptr)
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
						SNovaAssignNew(FastForwardButton, SNovaButton)
						.Text(LOCTEXT("FastForward", "Fast forward"))
						.HelpText(LOCTEXT("FastForwardHelp", "Wait until the next event"))
						.OnClicked(this, &SNovaMainMenuFlight::FastForward)
						.Enabled(this, &SNovaMainMenuFlight::CanFastForward)
					]
			
#if WITH_EDITOR && 1
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TimeDilation", "Disable time dilation"))
						.HelpText(LOCTEXT("TimeDilationHelp", "Set time dilation to zero"))
						.OnClicked(FSimpleDelegate::CreateLambda([this]()
						{
							GameState->SetTimeDilation(ENovaTimeDilation::Normal);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							return GameState && GameState->CanDilateTime(ENovaTimeDilation::Normal);
						})))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TimeDilation1", "Time dilation 1"))
						.HelpText(LOCTEXT("TimeDilation1Help", "Set time dilation to 1 (1s = 1m)"))
						.OnClicked(FSimpleDelegate::CreateLambda([this]()
						{
							GameState->SetTimeDilation(ENovaTimeDilation::Level1);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level1);
						})))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TimeDilation2", "Time dilation 2"))
						.HelpText(LOCTEXT("TimeDilation2Help", "Set time dilation to 2 (1s = 20m)"))
						.OnClicked(FSimpleDelegate::CreateLambda([this]()
						{
							GameState->SetTimeDilation(ENovaTimeDilation::Level2);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level2);
						})))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TestJoin", "Join random session"))
						.HelpText(LOCTEXT("TestJoinHelp", "Join random session"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							PC->TestJoin();
						}))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TestSilentRunning", "Silent running"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							MenuManager->SetInterfaceColor(Theme.NegativeColor, FLinearColor(1.0f, 0.0f, 0.1f));
						}))
					]

#endif // WITH_EDITOR
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaAssignNew(AlignManeuverButton, SNovaButton)
						.Text(LOCTEXT("AlignManeuver", "Align to maneuver"))
						.HelpText(LOCTEXT("AlignManeuverHelp", "Allow thrusters to fire to re-orient the spacecraft"))
						.OnClicked(this, &SNovaMainMenuFlight::OnAlignToManeuver)
						.Enabled(this, &SNovaMainMenuFlight::IsManeuveringEnabled)
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaAssignNew(MainDriveButton, SNovaButton)
						.Text(LOCTEXT("MainDriveButton", "Enable main drive"))
						.HelpText(LOCTEXT("MainDriveButtonHelp", "Allow the main drive to fire for maneuvers"))
						.OnClicked(this, &SNovaMainMenuFlight::OnEnableMainDrive)
						.Enabled(this, &SNovaMainMenuFlight::IsMainDriveAvailable)
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
	GameState          = IsValid(PC) ? PC->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
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
	else if (IsDockEnabled())
	{
		return DockButton;
	}
	else if (IsManeuveringEnabled())
	{
		return AlignManeuverButton;
	}
	else if (IsMainDriveAvailable())
	{
		return MainDriveButton;
	}
	else if (CanFastForward())
	{
		return FastForwardButton;
	}

	return nullptr;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::CanFastForward() const
{
	const ANovaGameMode* GameMode = MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>();
	return IsValid(GameMode) && GameMode->CanFastForward();
}

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return IsValid(PC) && IsValid(SpacecraftPawn) && !SpacecraftPawn->HasModifications() && SpacecraftPawn->IsSpacecraftValid() &&
		   SpacecraftMovement->CanUndock();
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return IsValid(PC) && IsValid(SpacecraftMovement) && SpacecraftMovement->CanDock();
}

bool SNovaMainMenuFlight::IsManeuveringEnabled() const
{
	if (OrbitalSimulation)
	{
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();

		return PlayerTrajectory && IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Idle &&
			   !SpacecraftMovement->IsAlignedToNextManeuver();
	}

	return false;
}

bool SNovaMainMenuFlight::IsMainDriveAvailable() const
{
	if (OrbitalSimulation)
	{
		const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();

		return PlayerTrajectory && IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Idle &&
			   SpacecraftMovement->IsAlignedToNextManeuver() && !SpacecraftMovement->IsMainDriveEnabled();
	}

	return false;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuFlight::FastForward()
{
	MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->FastForward();
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

void SNovaMainMenuFlight::OnAlignToManeuver()
{
	if (IsValid(SpacecraftMovement))
	{
		SpacecraftMovement->AlignToNextManeuver();
	}
}

void SNovaMainMenuFlight::OnEnableMainDrive()
{
	if (IsValid(SpacecraftMovement))
	{
		SpacecraftMovement->EnableMainDrive();
	}
}

#undef LOCTEXT_NAMESPACE
