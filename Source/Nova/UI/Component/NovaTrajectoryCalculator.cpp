// Nova project - Gwennaël Arbona

#include "NovaTrajectoryCalculator.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Spacecraft/NovaSpacecraft.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "Nova/System/NovaMenuManager.h"

#include "Nova/UI/Widget/NovaNavigationPanel.h"
#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaSlider.h"

#include "Nova/Nova.h"

#include "Widgets/Colors/SComplexGradient.h"

#define LOCTEXT_NAMESPACE "SNovaTrajectoryCalculator"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

SNovaTrajectoryCalculator::SNovaTrajectoryCalculator() : CurrentTrajectoryDisplayTime(0), NeedTrajectoryDisplayUpdate(false)
{}

void SNovaTrajectoryCalculator::Construct(const FArguments& InArgs)
{
	// Get data
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();
	MenuManager                         = InArgs._MenuManager;
	CurrentAlpha                        = InArgs._CurrentAlpha;
	TrajectoryFadeTime                  = InArgs._FadeTime;
	OnTrajectoryChanged                 = InArgs._OnTrajectoryChanged;
	AltitudeStep                        = 2;

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)

		// Title
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("TrajectoryTitle", "Trajectory"))
		]

		// Controls
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				InArgs._Panel->SNovaNew(SNovaButton)
				.Text(LOCTEXT("MinimizeDeltaV", "Minimize propellant"))
				.HelpText(LOCTEXT("MinimizeDeltaVHelp", "Configure the trajectory to minimize the delta-v cost"))
				.Action(InArgs._DeltaVActionName)
				.OnClicked(this, &SNovaTrajectoryCalculator::OptimizeForDeltaV)
				.Enabled(this, &SNovaTrajectoryCalculator::CanEditTrajectory)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				InArgs._Panel->SNovaNew(SNovaButton)
				.Text(LOCTEXT("MinimizeTravelTime", "Minimize travel time"))
				.HelpText(LOCTEXT("MinimizeTravelTimeHelp", "Configure the trajectory to minimize the travel time"))
				.Action(InArgs._DurationActionName)
				.OnClicked(this, &SNovaTrajectoryCalculator::OptimizeForDuration)
				.Enabled(this, &SNovaTrajectoryCalculator::CanEditTrajectory)
			]
		]

		// Slider & heatmaps
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			InArgs._Panel->SNovaAssignNew(Slider, SNovaSlider)
			.Size("DoubleButtonSize")
			.Value((InArgs._MaxAltitude - InArgs._MinAltitude) / 2)
			.MinValue(InArgs._MinAltitude)
			.MaxValue(InArgs._MaxAltitude)
			.ValueStep(50)
			.Analog(true)
			.HelpText(LOCTEXT("AltitudeSliderHelp", "Change the intermediate altitude used to synchronize orbits"))
			.Enabled(this, &SNovaTrajectoryCalculator::CanEditTrajectory)
			.Header()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				[
					SNew(SBox)
					.HeightOverride(16)
					[
						SNew(SBorder)
						.BorderImage(&ButtonTheme.Border)
						.BorderBackgroundColor(this, &SNovaTrajectoryCalculator::GetBorderColor)
						[
							SNew(SComplexGradient)
							.GradientColors(TAttribute<TArray<FLinearColor>>::Create(TAttribute<TArray<FLinearColor>>::FGetter::CreateSP(this, &SNovaTrajectoryCalculator::GetDeltaVGradient)))
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				[
					SNew(SBox)
					.HeightOverride(16)
					[
						SNew(SBorder)
						.BorderImage(&ButtonTheme.Border)
						.BorderBackgroundColor(this, &SNovaTrajectoryCalculator::GetBorderColor)
						[
							SNew(SComplexGradient)
							.GradientColors(TAttribute<TArray<FLinearColor>>::Create(TAttribute<TArray<FLinearColor>>::FGetter::CreateSP(this, &SNovaTrajectoryCalculator::GetDurationGradient)))
						]
					]
				]
			]
			.OnValueChanged(this, &SNovaTrajectoryCalculator::OnAltitudeSliderChanged)
		]

		// Main metrics
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.HeadingFont)
				.Text(this, &SNovaTrajectoryCalculator::GetDeltaVText)
			]

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(&Theme.HeadingFont)
				.Text(this, &SNovaTrajectoryCalculator::GetDurationText)
			]
		]

		// Fuel use
		+ SVerticalBox::Slot()
		.Padding(Theme.VerticalContentPadding)
		[
			SAssignNew(PropellantText, STextBlock)
			.TextStyle(&Theme.MainFont)
			.WrapTextAt(500)
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void SNovaTrajectoryCalculator::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update trajectory data
	if (NeedTrajectoryDisplayUpdate)
	{
		CurrentTrajectoryDisplayTime -= DeltaTime;
		if (CurrentTrajectoryDisplayTime <= 0)
		{
			NLOG("SNovaTrajectoryCalculator::Tick : updating trajectories");

			// Generate the gradients
			TrajectoryDeltaVGradientData.Empty();
			TrajectoryDurationGradientData.Empty();
			for (TPair<float, TSharedPtr<FNovaTrajectory>>& AltitudeAndTrajectory : SimulatedTrajectories)
			{
				TSharedPtr<FNovaTrajectory>& Trajectory = AltitudeAndTrajectory.Value;

				if (Trajectory.IsValid())
				{
					auto Transform = [](float Value)
					{
						return FMath::LogX(5, Value);
					};

					double DeltaVAlpha = FMath::Clamp(
						(Transform(Trajectory->TotalDeltaV) - Transform(MinDeltaV)) / (Transform(MaxDeltaV) - Transform(MinDeltaV)), 0.0f,
						1.0f);
					double DurationAlpha = FMath::Clamp((Transform(Trajectory->TotalTravelDuration.AsMinutes()) - Transform(MinDuration)) /
															(Transform(MaxDuration) - Transform(MinDuration)),
						0.0f, 1.0f);

					TrajectoryDeltaVGradientData.Add(FNovaStyleSet::GetPlasmaColor(DeltaVAlpha));
					TrajectoryDurationGradientData.Add(FNovaStyleSet::GetViridisColor(DurationAlpha));
				}
				else
				{
					TrajectoryDeltaVGradientData.Add(FLinearColor::Black);
					TrajectoryDurationGradientData.Add(FLinearColor::Black);
				}
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
	float CurrentTrajectoryAlpha = CurrentAlpha.Get(1.0f);
	CurrentTrajectoryDisplayTime = FMath::Clamp(CurrentTrajectoryDisplayTime, 0.0f, TrajectoryFadeTime);
	if (TrajectoryFadeTime > 0)
	{
		CurrentTrajectoryAlpha *= FMath::InterpEaseInOut(
			0.0f, 1.0f, FMath::Clamp(CurrentTrajectoryDisplayTime / TrajectoryFadeTime, 0.0f, 1.0f), ENovaUIConstants::EaseStandard);
	}

	if (TrajectoryDeltaVGradientData.Num() > 2)
	{
		for (FLinearColor& Color : TrajectoryDeltaVGradientData)
		{
			Color.A = CurrentTrajectoryAlpha;
		}
		for (FLinearColor& Color : TrajectoryDurationGradientData)
		{
			Color.A = CurrentTrajectoryAlpha;
		}
	}
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaTrajectoryCalculator::Reset()
{
	FLinearColor Translucent = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	SimulatedTrajectories          = {};
	TrajectoryDeltaVGradientData   = {Translucent, Translucent};
	TrajectoryDurationGradientData = {Translucent, Translucent};

	MinDeltaV              = FLT_MAX;
	MinDeltaVWithTolerance = FLT_MAX;
	MaxDeltaV              = 0;
	MinDuration            = FLT_MAX;
	MaxDuration            = 0;
}

void SNovaTrajectoryCalculator::SimulateTrajectories(const TSharedPtr<struct FNovaOrbit>& Source,
	const TSharedPtr<struct FNovaOrbit>& Destination, const TArray<FGuid>& SpacecraftIdentifiers)
{
	NCHECK(Source.IsValid());
	NCHECK(Destination.IsValid());

	int64 Cycles = FPlatformTime::Cycles64();

	// Get the game state
	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameState->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	Reset();

	// Run trajectory calculations over a range of altitudes
	const TSharedPtr<FNovaTrajectoryParameters>& Parameters =
		OrbitalSimulation->PrepareTrajectory(Source, Destination, FNovaTime::FromMinutes(1), SpacecraftIdentifiers);
	SimulatedTrajectories.Reserve((Slider->GetMaxValue() - Slider->GetMinValue()) / AltitudeStep + 1);
	for (float Altitude = Slider->GetMinValue(); Altitude <= Slider->GetMaxValue(); Altitude += AltitudeStep)
	{
		TSharedPtr<FNovaTrajectory> Trajectory = OrbitalSimulation->ComputeTrajectory(Parameters, Altitude);
		SimulatedTrajectories.Add(Altitude, Trajectory);
	}

	// Pre-process the trajectory data for absolute minimas and maximas
	for (const TPair<float, TSharedPtr<FNovaTrajectory>>& AltitudeAndTrajectory : SimulatedTrajectories)
	{
		float                              Altitude   = AltitudeAndTrajectory.Key;
		const TSharedPtr<FNovaTrajectory>& Trajectory = AltitudeAndTrajectory.Value;

		if (Trajectory.IsValid())
		{
			double TotalTravelDuration = Trajectory->TotalTravelDuration.AsMinutes();

			if (FMath::IsFinite(Trajectory->TotalDeltaV) && FMath::IsFinite(TotalTravelDuration))
			{
				if (Trajectory->TotalDeltaV < MinDeltaV)
				{
					MinDeltaV = Trajectory->TotalDeltaV;
				}
				if (Trajectory->TotalDeltaV > MaxDeltaV)
				{
					MaxDeltaV = Trajectory->TotalDeltaV;
				}
				if (TotalTravelDuration < MinDuration)
				{
					MinDuration         = TotalTravelDuration;
					MinDurationAltitude = Altitude;
				}
				if (TotalTravelDuration > MaxDuration)
				{
					MaxDuration = TotalTravelDuration;
				}
			}
		}
	}

	// Pre-process the trajectory data again for a smarter minimum delta-V
	float MinDurationWithinMinDeltaV = FLT_MAX;
	for (const TPair<float, TSharedPtr<FNovaTrajectory>>& AltitudeAndTrajectory : SimulatedTrajectories)
	{
		float                              Altitude   = AltitudeAndTrajectory.Key;
		const TSharedPtr<FNovaTrajectory>& Trajectory = AltitudeAndTrajectory.Value;

		if (Trajectory.IsValid() && FMath::IsFinite(Trajectory->TotalDeltaV) &&
			FMath::IsFinite(Trajectory->TotalTravelDuration.AsMinutes()))
		{
			if (Trajectory->TotalDeltaV < 1.001f * MinDeltaV && Trajectory->TotalTravelDuration.AsMinutes() < MinDurationWithinMinDeltaV)
			{
				MinDeltaVWithTolerance     = Trajectory->TotalDeltaV;
				MinDurationWithinMinDeltaV = Trajectory->TotalTravelDuration.AsMinutes();
				MinDeltaVAltitude          = Altitude;
			}
		}
	}

	// Complete display setup
	NeedTrajectoryDisplayUpdate = true;
	OptimizeForDeltaV();

	NLOG("NovaTrajectoryCalculator::SimulateTrajectories : simulated %d trajectories in %.2fms", SimulatedTrajectories.Num(),
		FPlatformTime::ToMilliseconds(FPlatformTime::Cycles64() - Cycles));
}

void SNovaTrajectoryCalculator::OptimizeForDeltaV()
{
	if (SimulatedTrajectories.Num())
	{
		NLOG("SNovaTrajectoryCalculator::OptimizeForDeltaV");

		Slider->SetCurrentValue(MinDeltaVAltitude);
		OnAltitudeSliderChanged(MinDeltaVAltitude);
	}
}

void SNovaTrajectoryCalculator::OptimizeForDuration()
{
	if (SimulatedTrajectories.Num())
	{
		NLOG("SNovaTrajectoryCalculator::OptimizeForDuration");

		Slider->SetCurrentValue(MinDurationAltitude);
		OnAltitudeSliderChanged(MinDurationAltitude);
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FSlateColor SNovaTrajectoryCalculator::GetBorderColor() const
{
	return MenuManager->GetInterfaceColor();
}

bool SNovaTrajectoryCalculator::CanEditTrajectory() const
{
	return SimulatedTrajectories.Num() > 0;
}

FText SNovaTrajectoryCalculator::GetDeltaVText() const
{
	const TSharedPtr<FNovaTrajectory>* TrajectoryPtr = SimulatedTrajectories.Find(CurrentAltitude);
	if (TrajectoryPtr)
	{
		const TSharedPtr<FNovaTrajectory>& Trajectory = *TrajectoryPtr;
		if (Trajectory.IsValid())
		{
			FNumberFormattingOptions NumberOptions;
			NumberOptions.SetMaximumFractionalDigits(1);

			return FText::FormatNamed(
				LOCTEXT("DeltaVFormat", "{deltav} m/s"), TEXT("deltav"), FText::AsNumber(Trajectory->TotalDeltaV, &NumberOptions));
		}
	}

	return LOCTEXT("InvalidDeltaV", "No trajectory");
}

FText SNovaTrajectoryCalculator::GetDurationText() const
{
	const TSharedPtr<FNovaTrajectory>* TrajectoryPtr = SimulatedTrajectories.Find(CurrentAltitude);
	if (TrajectoryPtr)
	{
		const TSharedPtr<FNovaTrajectory>& Trajectory = *TrajectoryPtr;
		if (Trajectory.IsValid())
		{
			return ::GetDurationText(Trajectory->TotalTravelDuration, 2);
		}
	}

	return LOCTEXT("InvalidDuration", "No trajectory");
}

void SNovaTrajectoryCalculator::OnAltitudeSliderChanged(float Altitude)
{
	CurrentAltitude =
		Slider->GetMinValue() + FMath::RoundToInt((Altitude - Slider->GetMinValue()) / static_cast<float>(AltitudeStep)) * AltitudeStep;

	FString TrajectoryDetails;

	const TSharedPtr<FNovaTrajectory>* TrajectoryPtr = SimulatedTrajectories.Find(CurrentAltitude);
	if (TrajectoryPtr)
	{
		bool HasEnoughPropellant = true;

		ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
		NCHECK(GameState);
		if (IsValid(GameState))
		{
			for (const FGuid& Identifier : GameState->GetPlayerSpacecraftIdentifiers())
			{
				const FNovaSpacecraft* Spacecraft = GameState->GetSpacecraft(Identifier);

				if (Spacecraft)
				{
					UNovaSpacecraftPropellantSystem* PropellantSystem =
						Spacecraft->FindComponentByClass<UNovaSpacecraftPropellantSystem>(GameState);
					NCHECK(PropellantSystem);

					// Process remaining propellant
					float FuelToBeUsed = Spacecraft->GetPropulsionMetrics().GetRequiredPropellant(
						(*TrajectoryPtr)->TotalDeltaV, Spacecraft->GetCurrentCargoMass());
					float FuelRemaining = PropellantSystem->GetCurrentPropellantMass();
					if (HasEnoughPropellant && FuelToBeUsed > FuelRemaining)
					{
						HasEnoughPropellant = false;
					}

					if (TrajectoryDetails.Len())
					{
						TrajectoryDetails += "\n";
					}
					TrajectoryDetails += TEXT("• ");

					FNumberFormattingOptions Options;
					Options.MaximumFractionalDigits = 1;

					// Format the propellant data
					TrajectoryDetails += FText::FormatNamed(
						LOCTEXT("TrajectoryFuelFormat", "{spacecraft}: {used} T of propellant required ({remaining} T remaining)"),
						TEXT("spacecraft"), Spacecraft->GetName(), TEXT("used"), FText::AsNumber(FuelToBeUsed, &Options), TEXT("remaining"),
						FText::AsNumber(FuelRemaining, &Options))
											 .ToString();
				}
			}
		}

		OnTrajectoryChanged.ExecuteIfBound(*TrajectoryPtr, HasEnoughPropellant);
	}

	PropellantText->SetText(FText::FromString(TrajectoryDetails));
}

#undef LOCTEXT_NAMESPACE
