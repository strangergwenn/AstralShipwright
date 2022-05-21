// Astral Shipwright - Gwennaël Arbona

#include "NovaEventDisplay.h"

#include "Game/NovaArea.h"
#include "Game/NovaGameModeStates.h"
#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "System/NovaAssetManager.h"
#include "System/NovaGameInstance.h"
#include "System/NovaMenuManager.h"

#include "Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "NovaEventDisplay"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaEventDisplay::SNovaEventDisplay()
{}

void SNovaEventDisplay::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Settings
	MenuManager = InArgs._MenuManager;

	// clang-format off
	SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
		.ColorAndOpacity(this, &SNovaEventDisplay::GetDisplayColor));

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0, FNovaStyleSet::GetButtonSize("TabButtonSize").Height + 30))
	[
		SNew(SBorder)
		.BorderImage(&Theme.MainMenuGenericBorder)
		.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
		.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
		.Padding(2)
		.Visibility(this, &SNovaEventDisplay::GetMainVisibility)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(Theme.BlurRadius)
			.BlurStrength(Theme.BlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBox)
				.WidthOverride(Theme.EventDisplayWidth)
				[
					SNew(SBorder)
					.HAlign(HAlign_Center)
					.BorderImage(&Theme.MainMenuGenericBackground)
					.Padding(Theme.ContentPadding)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.HeadingFont)
							.Text(this, &SNovaEventDisplay::GetMainText)
							.WrapTextAt(Theme.EventDisplayWidth)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SNovaImage)
								.Image(FNovaImageGetter::CreateSP(this, &SNovaEventDisplay::GetIcon))
								.Visibility(this, &SNovaEventDisplay::GetDetailsVisibility)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SNovaText)
								.TextStyle(&Theme.MainFont)
								.Text(FNovaTextGetter::CreateSP(this, &SNovaEventDisplay::GetDetailsText))
								.WrapTextAt(Theme.EventDisplayWidth)
								.Visibility(this, &SNovaEventDisplay::GetDetailsVisibility)
							]
						]
					]
				]
			]
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaEventDisplay::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaFadingWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	DesiredState = FNovaEventDisplayData();
	DetailsText  = FText();

	if (MenuManager.IsValid())
	{
		auto PC                 = MenuManager->GetPC();
		auto SpacecraftPawn     = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
		auto SpacecraftMovement = IsValid(SpacecraftPawn) ? SpacecraftPawn->GetSpacecraftMovement() : nullptr;
		auto GameState          = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
		auto OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;

		if (IsValid(PC) && IsValid(SpacecraftPawn) && IsValid(GameState) && IsValid(OrbitalSimulation) &&
			PC->GetCameraState() == ENovaPlayerCameraState::Default)
		{
			if (SpacecraftPawn->IsDocked())
			{
				if (SpacecraftPawn->HasModifications())
				{
					DesiredState.Text = LOCTEXT("ModifiedSpacecraft", "Spacecraft has pending changes");
				}
			}
			else
			{
				const FNovaTrajectory* Trajectory      = OrbitalSimulation->GetPlayerTrajectory();
				const FNovaTime&       CurrentGameTime = GameState->GetCurrentTime();

				// Trajectory
				if (Trajectory && Trajectory->GetArrivalTime() > CurrentGameTime)
				{
					// Ongoing maneuver
					const FNovaManeuver* CurrentManeuver = Trajectory->GetManeuver(CurrentGameTime);
					if (CurrentManeuver)
					{
						FNovaTime TimeLeftInManeuver = (CurrentManeuver->Time + CurrentManeuver->Duration - CurrentGameTime);

						DesiredState.Text = FText::FormatNamed(LOCTEXT("CurrentManeuverFormat", "Maneuver ends in {duration}"),
							TEXT("duration"), GetDurationText(TimeLeftInManeuver));
						DesiredState.Type = ENovaEventDisplayType::DynamicText;
					}

					// Nearing maneuver
					else
					{
						FNovaTime TimeBeforeNextManeuver = Trajectory->GetNextManeuverStartTime(CurrentGameTime) - CurrentGameTime;

						DesiredState.Text = FText::FormatNamed(LOCTEXT("NextManeuverFormat", "Next maneuver in {duration}"),
							TEXT("duration"), GetDurationText(TimeBeforeNextManeuver));

						DesiredState.Type = ENovaEventDisplayType::DynamicText;

						if (CurrentState.Type == ENovaEventDisplayType::DynamicText)
						{
							IsValidDetails = SpacecraftMovement->CanManeuver();
							if (IsValidDetails)
							{
								DetailsText = FText::FormatNamed(LOCTEXT("ImminentManeuverAuthorized",
																	 "{spacecraft}|plural(one=This,other=All) spacecraft "
																	 "{spacecraft}|plural(one=is,other=are) ready to maneuver"),
									TEXT("spacecraft"), GameState->PlayerArray.Num());
							}
							else
							{
								DetailsText =
									FText::FormatNamed(LOCTEXT("ImminentManeuverUnauthorized",
														   "{spacecraft}|plural(one=This,other=A) spacecraft isn't ready to maneuver"),
										TEXT("spacecraft"), GameState->PlayerArray.Num());
							}
						}
					}
				}

				// Free flight
				else
				{
					if (GameState->GetCurrentArea()->Hidden)
					{
						DesiredState.Text = LOCTEXT("InOrbit", "In orbit");
					}
					else
					{
						DesiredState.Text = FText::FormatNamed(
							LOCTEXT("FreeFlightFormat", "In orbit at {station}"), TEXT("station"), GameState->GetCurrentArea()->Name);
					}
				}
			}
		}
	}

	// Dynamic text bypasses the dirty system
	if (CurrentState.Type == DesiredState.Type && CurrentState.Type == ENovaEventDisplayType::DynamicText)
	{
		CurrentState.Text = DesiredState.Text;
	}
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

EVisibility SNovaEventDisplay::GetMainVisibility() const
{
	return CurrentState.Text.IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}

EVisibility SNovaEventDisplay::GetDetailsVisibility() const
{
	return DetailsText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

FLinearColor SNovaEventDisplay::GetDisplayColor() const
{
	return 1.25f * MenuManager->GetHighlightColor();
}

FText SNovaEventDisplay::GetMainText() const
{
	return CurrentState.Text;
}

FText SNovaEventDisplay::GetDetailsText() const
{
	return DetailsText;
}

const FSlateBrush* SNovaEventDisplay::GetIcon() const
{
	return IsValidDetails ? FNovaStyleSet::GetBrush("Icon/SB_On") : FNovaStyleSet::GetBrush("Icon/SB_Warning");
}

#undef LOCTEXT_NAMESPACE
