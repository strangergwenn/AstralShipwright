// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/UI/Component/NovaOrbitalMap.h"
#include "Nova/UI/Widget/NovaSlider.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Colors/SComplexGradient.h"
#include "Slate/SRetainerWidget.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;
	TrajectoryDisplayFadeTime   = ENovaUIConstants::FadeDurationShort;

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
							MenuManager->GetPC()->TestJoin();
						}))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("LeaveStation", "Leave station"))
						.HelpText(LOCTEXT("HelpLeaveStation", "Leave station"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							const class UNovaArea* Orbit = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{D1D46588-494D-E081-ADE6-48AE0B010BBB}"));
							MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChangeArea(Orbit);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							return MenuManager->GetPC() && MenuManager->GetPC()->GetLocalRole() == ROLE_Authority;
						})))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaNew(SNovaButton)
						.Text(LOCTEXT("GoToStation", "Go to station"))
						.HelpText(LOCTEXT("HelpGoToStation", "Go to station"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							const class UNovaArea* Station = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
							MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChangeArea(Station);
						}))
						.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
						{
							return MenuManager->GetPC() && MenuManager->GetPC()->GetLocalRole() == ROLE_Authority;
						})))
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
						.Text(LOCTEXT("ComputeTrajectory200", "Compute 200km trajectory"))
						.HelpText(LOCTEXT("HelpComputeTrajectory200", "Compute trajectory"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							const class UNovaArea* StationA = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
							const class UNovaArea* StationB = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CCA2E0C7-43AE-CDD1-06CA-AF951F61C44A}"));
							const class UNovaArea* StationC = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CAC5C9B9-451B-1212-6EC4-E8918B69A795}"));
					
							ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
							NCHECK(GameState);
							ANovaGameWorld* GameWorld = GameState->GetGameWorld();
							NCHECK(GameWorld);
							UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
							NCHECK(OrbitalSimulation);

							OrbitalMap->PreviewTrajectory(OrbitalSimulation->ComputeTrajectory(StationA, StationC, 200));
						}))
					]
			
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNovaAssignNew(UndockButton, SNovaButton)
						.Text(LOCTEXT("DumpTrajectories", "Dump trajectories"))
						.HelpText(LOCTEXT("HelpDumpTrajectories", "Dump trajectories to log"))
						.OnClicked(FSimpleDelegate::CreateLambda([&]()
						{
							const class UNovaArea* StationA = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
							const class UNovaArea* StationB = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CCA2E0C7-43AE-CDD1-06CA-AF951F61C44A}"));
							const class UNovaArea* StationC = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CAC5C9B9-451B-1212-6EC4-E8918B69A795}"));
					
							SimulateTrajectories(StationA, StationC);
						}))
					]
			
					// Delta-V trade-off slider
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(Theme.ContentPadding)
					[
						SNew(SNovaSlider)
						.Panel(this)
						.Size("LargeSliderSize")
						.Value(500)
						.MinValue(200)
						.MaxValue(1000)
						.ValueStep(50)
						.Analog(true)
						.HelpText(LOCTEXT("AltitudeSliderHelp", "Change the intermediate altitude used to synchronize orbits"))
						.Header()
						[
							SNew(SBox)
							.HeightOverride(32)
							.Padding(Theme.VerticalContentPadding)
							[
								SNew(SComplexGradient)
								.GradientColors(TAttribute<TArray<FLinearColor>>::Create(TAttribute<TArray<FLinearColor>>::FGetter::CreateSP(this, &SNovaMainMenuFlight::GetDeltaVGradient)))
							]
						]
						.Footer()
						[
							SNew(SBox)
							.HeightOverride(32)
							.Padding(Theme.VerticalContentPadding)
							[
								SNew(SComplexGradient)
								.GradientColors(TAttribute<TArray<FLinearColor>>::Create(TAttribute<TArray<FLinearColor>>::FGetter::CreateSP(this, &SNovaMainMenuFlight::GetDurationGradient)))
							]
						]
						.OnValueChanged(this, &SNovaMainMenuFlight::OnAltitudeSliderChanged)
					]
				]
			]

			// Map slot
			+ SHorizontalBox::Slot()
			[
				SAssignNew(MapRetainer, SRetainerWidget)
				[
					SAssignNew(OrbitalMap, SNovaOrbitalMap)
					.MenuManager(MenuManager)
				]
			]
		]
	];
	// clang-format on

	// Setup retainer
	MapRetainer->SetEffectMaterial(Theme.EffectMaterial);
	MapRetainer->SetTextureParameter(TEXT("UI"));
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update trajectory data
	if (NeedTrajectoryDisplayUpdate)
	{
		CurrentTrajectoryDisplayTime -= DeltaTime;
		if (CurrentTrajectoryDisplayTime < 0)
		{
			NLOG("NovaMainMenuFlight::Tick : updating trajectories");

			// Pre-process the trajectory data
			float MinDeltaV = FLT_MAX, MaxDeltaV = 0, MinDuration = FLT_MAX, MaxDuration = 0;
			for (FNovaTrajectoryCharacteristics& Res : SimulatedTrajectories)
			{
				Res.DeltaV   = FMath::LogX(10, Res.DeltaV);
				Res.Duration = FMath::LogX(10, Res.Duration);

				if (Res.DeltaV < MinDeltaV)
				{
					MinDeltaV = Res.DeltaV;
				}
				if (Res.DeltaV > MaxDeltaV && FMath::IsFinite(Res.DeltaV))
				{
					MaxDeltaV = Res.DeltaV;
				}
				if (Res.Duration < MinDuration)
				{
					MinDuration = Res.Duration;
				}
				if (Res.Duration > MaxDuration && FMath::IsFinite(Res.Duration))
				{
					MaxDuration = Res.Duration;
				}
			}

			// Generate the gradients
			TrajectoryDeltaVGradientData.Empty();
			TrajectoryDurationGradientData.Empty();
			for (const FNovaTrajectoryCharacteristics& Res : SimulatedTrajectories)
			{
				double DeltaVAlpha   = FMath::Clamp((Res.DeltaV - MinDeltaV) / (MaxDeltaV - MinDeltaV), 0.0, 1.0);
				double DurationAlpha = FMath::Clamp((Res.Duration - MinDuration) / (MaxDuration - MinDuration), 0.0, 1.0);
				TrajectoryDeltaVGradientData.Add(FNovaStyleSet::GetPlasmaColor(DeltaVAlpha));
				TrajectoryDurationGradientData.Add(FNovaStyleSet::GetViridisColor(DurationAlpha));
			}

			CurrentTrajectoryDisplayTime = 0;
			NeedTrajectoryDisplayUpdate  = false;
		}
	}
	else
	{
		CurrentTrajectoryDisplayTime += DeltaTime;
	}

	// Update the alpha of the gradients
	CurrentTrajectoryDisplayTime = FMath::Clamp(CurrentTrajectoryDisplayTime, 0.0f, TrajectoryDisplayFadeTime);
	float CurrentTrajectoryAlpha = CurrentAlpha * FMath::Clamp(CurrentTrajectoryDisplayTime / TrajectoryDisplayFadeTime, 0.0f, 1.0f);
	for (FLinearColor& Color : TrajectoryDeltaVGradientData)
	{
		Color.A = CurrentTrajectoryAlpha;
	}
	for (FLinearColor& Color : TrajectoryDurationGradientData)
	{
		Color.A = CurrentTrajectoryAlpha;
	}
}

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();

	GetSpacecraftPawn()->SetHighlightCompartment(INDEX_NONE);

	// Clear trajectory data
	SimulatedTrajectories          = {};
	TrajectoryDeltaVGradientData   = {FLinearColor::Black, FLinearColor::Black};
	TrajectoryDurationGradientData = {FLinearColor::Black, FLinearColor::Black};
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
	if (SpacecraftPawn)
	{
		return Cast<UNovaSpacecraftMovementComponent>(SpacecraftPawn->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
	}
	else
	{
		return nullptr;
	}
}

void SNovaMainMenuFlight::SimulateTrajectories(const class UNovaArea* Source, const class UNovaArea* Destination)
{
	NCHECK(Source);
	NCHECK(Destination);
	NLOG("NovaMainMenuFlight::SimulateTrajectories : %s -> %s", *Source->Name.ToString(), *Destination->Name.ToString());

	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Run trajectory calculations over a range of altitudes
	SimulatedTrajectories.Empty();
	for (float Altitude = 200; Altitude <= 1000; Altitude += 5)
	{
		TSharedPtr<FNovaTrajectory>    Trajectory = OrbitalSimulation->ComputeTrajectory(Source, Destination, Altitude);
		FNovaTrajectoryCharacteristics TrajectoryCharacteristics;
		TrajectoryCharacteristics.IntermediateAltitude = Altitude;
		TrajectoryCharacteristics.Duration             = Trajectory->TotalTransferDuration;
		TrajectoryCharacteristics.DeltaV               = Trajectory->TotalDeltaV;
		SimulatedTrajectories.Add(TrajectoryCharacteristics);
	}

	NeedTrajectoryDisplayUpdate = true;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->GetState() == ENovaMovementState::Docked;
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->GetState() == ENovaMovementState::Idle;
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

	MenuManager->GetPC()->Dock();
}

void SNovaMainMenuFlight::OnAltitudeSliderChanged(float Altitude)
{
	const class UNovaArea* StationA =
		MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
	const class UNovaArea* StationB =
		MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CCA2E0C7-43AE-CDD1-06CA-AF951F61C44A}"));
	const class UNovaArea* StationC =
		MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CAC5C9B9-451B-1212-6EC4-E8918B69A795}"));

	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	OrbitalMap->PreviewTrajectory(OrbitalSimulation->ComputeTrajectory(StationA, StationC, Altitude), true);
}

#undef LOCTEXT_NAMESPACE
