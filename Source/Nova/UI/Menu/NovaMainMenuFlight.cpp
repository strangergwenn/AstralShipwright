// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/UI/Component/NovaOrbitalMap.h"
#include "Nova/UI/Component/NovaTrajectoryCalculator.h"
#include "Nova/UI/Widget/NovaSlider.h"
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
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TestJoin", "Join random session"))
						.HelpText(LOCTEXT("HelpTestJoin", "Join random session"))
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
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText {
							const UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

							if (OrbitalSimulation)
							{
								const FNovaTrajectory* Trajectory = OrbitalSimulation->GetPlayerTrajectory();
								const FNovaOrbitalLocation* Location = OrbitalSimulation->GetPlayerLocation();

								if (Trajectory)
								{
									return LOCTEXT("Trajectory", "On trajectory");
								}
								else if (Location)
								{
									auto AreaAndDistance = OrbitalSimulation->GetNearestAreaAndDistance(*Location);

									FNumberFormattingOptions NumberOptions;
									NumberOptions.SetMaximumFractionalDigits(1);

									return FText::FormatNamed(LOCTEXT("NearestAreaFormat", "{area} at {distance}km"),
										TEXT("area"), AreaAndDistance.Key->Name,
										TEXT("distance"), FText::AsNumber(AreaAndDistance.Value, &NumberOptions));
								}
							}
							return FText();
						})
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
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaAssignNew(DestinationListView, SNovaModalListView<const UNovaArea*>)
						.Panel(this)
						.TitleText(LOCTEXT("DestinationList", "Destinations"))
						.HelpText(LOCTEXT("DestinationListHelp", "Plan a trajectory toward a destination"))
						.ItemsSource(&DestinationList)
						.OnGenerateItem(this, &SNovaMainMenuFlight::GenerateDestinationItem)
						.OnGenerateName(this, &SNovaMainMenuFlight::GetDestinationName)
						.OnGenerateTooltip(this, &SNovaMainMenuFlight::GenerateDestinationTooltip)
						.OnSelectionChanged(this, &SNovaMainMenuFlight::OnSelectedDestinationChanged)
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("CommitTrajectory", "Commit trajectory"))
						.HelpText(LOCTEXT("CommitTrajectoryHelp", "Commit to the currently selected trajectory"))
						.OnClicked(this, &SNovaMainMenuFlight::OnCommitTrajectory)
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

							return OrbitalSimulation && OrbitalSimulation->CanStartTrajectory(OrbitalMap->GetPreviewTrajectory()) && SelectedDestination != GameState->GetCurrentArea();
						})))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TimeDilation", "Disable time dilation"))
						.HelpText(LOCTEXT("TimeDilationHelp", "Set time dilation to zero"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							GameState->SetTimeDilation(ENovaTimeDilation::Normal);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							return GameState && GameState->CanDilateTime(ENovaTimeDilation::Normal);
						})))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("TimeDilation1", "Time dilation 1"))
						.HelpText(LOCTEXT("TimeDilation1Help", "Set time dilation to 1 (1s = 1m)"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
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
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							GameState->SetTimeDilation(ENovaTimeDilation::Level2);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level2);
						})))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("FastForward", "Fast forward"))
						.HelpText(LOCTEXT("FastForwardHelp", "Wait until the next event"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							MenuManager->GetPC()->GetWorld()->GetAuthGameMode<ANovaGameMode>()->FastForward();
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							const ANovaGameMode* GameMode = MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>();
							return GameMode && GameMode->CanFastForward();
						})))
					]
			
					// Delta-V trade-off slider
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(Theme.ContentPadding)
					[
						SAssignNew(TrajectoryCalculator, SNovaTrajectoryCalculator)
						.MenuManager(MenuManager)
						.Panel(this)
						.CurrentAlpha(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateSP(this, &SNovaTabPanel::GetCurrentAlpha)))
						.DeltaVActionName(FNovaPlayerInput::MenuPrimary)
						.DurationActionName(FNovaPlayerInput::MenuSecondary)
						.OnTrajectoryChanged(this, &SNovaMainMenuFlight::OnTrajectoryChanged)
					]
				]
			]

			// Map slot
			+ SHorizontalBox::Slot()
			[
				SAssignNew(OrbitalMap, SNovaOrbitalMap)
				.MenuManager(MenuManager)
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

	GetSpacecraftPawn()->SetHighlightCompartment(INDEX_NONE);

	TrajectoryCalculator->Reset();

	// Destination list
	SelectedDestination = nullptr;
	DestinationList     = MenuManager->GetGameInstance()->GetAssetManager()->GetAssets<UNovaArea>();
	DestinationListView->Refresh();
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
    Callbacks (destination)
----------------------------------------------------*/

TSharedRef<SWidget> SNovaMainMenuFlight::GenerateDestinationItem(const UNovaArea* Destination)
{
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// clang-format off
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.Padding(ButtonTheme.IconPadding)
		.AutoWidth()
		[
			SNew(SBorder)
			.BorderImage(new FSlateNoResource)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &SNovaMainMenuFlight::GetDestinationIcon, Destination)
			]
		]

		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.MainFont)
			.Text(GetDestinationName(Destination))
		];
	// clang-format on
}

FText SNovaMainMenuFlight::GetDestinationName(const UNovaArea* Destination) const
{
	return Destination->Name;
}

const FSlateBrush* SNovaMainMenuFlight::GetDestinationIcon(const UNovaArea* Destination) const
{
	return FNovaStyleSet::GetBrush(Destination == SelectedDestination ? "Icon/SB_ListOn" : "Icon/SB_ListOff");
}

FText SNovaMainMenuFlight::GenerateDestinationTooltip(const UNovaArea* Destination)
{
	return FText::FormatNamed(LOCTEXT("DestinationHelp", "Set course for {area}"), TEXT("area"), Destination->Name);
}

void SNovaMainMenuFlight::OnSelectedDestinationChanged(const UNovaArea* Destination, int32 Index)
{
	NLOG("SNovaMainMenuFlight::OnSelectedDestinationChanged : %s", *Destination->Name.ToString());

	SelectedDestination = Destination;

	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);

	if (SelectedDestination != GameState->GetCurrentArea())
	{
		UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

		TrajectoryCalculator->SimulateTrajectories(MakeShared<FNovaOrbit>(*OrbitalSimulation->GetPlayerOrbit()),
			OrbitalSimulation->GetAreaOrbit(Destination), GameState->GetPlayerSpacecraftIdentifiers());
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

void SNovaMainMenuFlight::OnTrajectoryChanged(TSharedPtr<FNovaTrajectory> Trajectory)
{
	OrbitalMap->Set(Trajectory, false);
}

void SNovaMainMenuFlight::OnCommitTrajectory()
{
	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

	const TSharedPtr<FNovaTrajectory>& Trajectory = OrbitalMap->GetPreviewTrajectory();
	if (OrbitalSimulation && OrbitalSimulation->CanStartTrajectory(Trajectory))
	{
		OrbitalSimulation->StartTrajectory(GameState->GetPlayerSpacecraftIdentifiers(), Trajectory);
		OrbitalMap->ClearTrajectoryPreview();
		TrajectoryCalculator->Reset();
	}
}

#undef LOCTEXT_NAMESPACE
