// Nova project - Gwennaël Arbona

#include "NovaTrajectoryCalculator.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

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
	const FNovaMainTheme&   Theme           = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme     = FNovaStyleSet::GetButtonTheme();
	const FNovaSliderTheme& SliderTheme     = FNovaStyleSet::GetTheme<FNovaSliderTheme>("DefaultSlider");
	const FSlateBrush*      BackgroundBrush = FNovaStyleSet::GetBrush("Game/SB_TrajectoryCalculator");
	MenuManager                             = InArgs._MenuManager;
	CurrentAlpha                            = InArgs._CurrentAlpha;
	OnTrajectoryChanged                     = InArgs._OnTrajectoryChanged;
	AltitudeStep                            = 2;

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(BackgroundBrush->GetImageSize().X)
			.HeightOverride(BackgroundBrush->GetImageSize().Y)
			[
				SNew(SBorder)
				.BorderImage(BackgroundBrush)
				.Padding(1)
				.RenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(45))))
				.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			]
		]

		+ SOverlay::Slot()
		[
			SAssignNew(Slider, SNovaSlider)
			.Panel(InArgs._Panel)
			.Size("LargeSliderSize")
			.Value((InArgs._MaxAltitude - InArgs._MinAltitude) / 2)
			.MinValue(InArgs._MinAltitude)
			.MaxValue(InArgs._MaxAltitude)
			.ValueStep(50)
			.Analog(true)
			.HelpText(LOCTEXT("AltitudeSliderHelp", "Change the intermediate altitude used to synchronize orbits"))
			.Header()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(this, &SNovaTrajectoryCalculator::GetDeltaVText)
				]

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					InArgs._Panel->SNovaNew(SNovaButton)
					.Text(LOCTEXT("MinimizeDeltaV", "Minimize Delta-V"))
					.HelpText(LOCTEXT("MinimizeDeltaVHelp", "Configure the trajectory to minimize the Delta-V cost"))
					.Action(InArgs._DeltaVActionName)
					.OnClicked(this, &SNovaTrajectoryCalculator::OptimizeForDeltaV)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding + FMargin(0, 0, 0, ButtonTheme.AnimationPadding.Top))
				[
					SNew(SBox)
					.HeightOverride(32)
					[
						SNew(SBorder)
						.BorderImage(&SliderTheme.Border)
						[
							SNew(SComplexGradient)
							.GradientColors(TAttribute<TArray<FLinearColor>>::Create(TAttribute<TArray<FLinearColor>>::FGetter::CreateSP(this, &SNovaTrajectoryCalculator::GetDeltaVGradient)))
						]
					]
				]
			]
			.Footer()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding + FMargin(0, ButtonTheme.AnimationPadding.Bottom, 0, 0))
				[
					SNew(SBox)
					.HeightOverride(32)
					[
						SNew(SBorder)
						.BorderImage(&SliderTheme.Border)
						[
							SNew(SComplexGradient)
							.GradientColors(TAttribute<TArray<FLinearColor>>::Create(TAttribute<TArray<FLinearColor>>::FGetter::CreateSP(this, &SNovaTrajectoryCalculator::GetDurationGradient)))
						]
					]
				]

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					InArgs._Panel->SNovaNew(SNovaButton)
					.Text(LOCTEXT("MinimizeTravelTime", "Minimize travel time"))
					.HelpText(LOCTEXT("MinimizeTravelTimeHelp", "Configure the trajectory to minimize the travel tile"))
					.Action(InArgs._DurationActionName)
					.OnClicked(this, &SNovaTrajectoryCalculator::OptimizeForDuration)
				]

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(&Theme.SubtitleFont)
					.Text(this, &SNovaTrajectoryCalculator::GetDurationText)
				]
			]
			.OnValueChanged(this, &SNovaTrajectoryCalculator::OnAltitudeSliderChanged)
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
		if (CurrentTrajectoryDisplayTime < 0)
		{
			NLOG("SNovaTrajectoryCalculator::Tick : updating trajectories");

			// Generate the gradients
			TrajectoryDeltaVGradientData.Empty();
			TrajectoryDurationGradientData.Empty();
			for (TPair<float, TSharedPtr<FNovaTrajectory>>& AltitudeAndTrajectory : SimulatedTrajectories)
			{
				TSharedPtr<FNovaTrajectory>& Trajectory = AltitudeAndTrajectory.Value;
				NCHECK(Trajectory.IsValid());

				auto Transform = [](float Value)
				{
					return FMath::LogX(5, Value);
				};

				double DeltaVAlpha = FMath::Clamp(
					(Transform(Trajectory->TotalDeltaV) - Transform(MinDeltaV)) / (Transform(MaxDeltaV) - Transform(MinDeltaV)), 0.0f,
					1.0f);
				double DurationAlpha = FMath::Clamp((Transform(Trajectory->TotalTransferDuration) - Transform(MinDuration)) /
														(Transform(MaxDuration) - Transform(MinDuration)),
					0.0f, 1.0f);

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
	CurrentTrajectoryDisplayTime = FMath::Clamp(CurrentTrajectoryDisplayTime, 0.0f, ENovaUIConstants::FadeDurationShort);
	float CurrentTrajectoryAlpha =
		CurrentAlpha.Get(1.0f) * FMath::Clamp(CurrentTrajectoryDisplayTime / ENovaUIConstants::FadeDurationShort, 0.0f, 1.0f);
	for (FLinearColor& Color : TrajectoryDeltaVGradientData)
	{
		Color.A = CurrentTrajectoryAlpha;
	}
	for (FLinearColor& Color : TrajectoryDurationGradientData)
	{
		Color.A = CurrentTrajectoryAlpha;
	}
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaTrajectoryCalculator::Reset()
{
	SimulatedTrajectories          = {};
	TrajectoryDeltaVGradientData   = {FLinearColor::Black, FLinearColor::Black};
	TrajectoryDurationGradientData = {FLinearColor::Black, FLinearColor::Black};

	MinDeltaV   = FLT_MAX;
	MaxDeltaV   = 0;
	MinDuration = FLT_MAX;
	MaxDuration = 0;
}

void SNovaTrajectoryCalculator::SimulateTrajectories(const class UNovaArea* Source, const class UNovaArea* Destination)
{
	NCHECK(Source);
	NCHECK(Destination);
	NLOG("SNovaTrajectoryCalculator::SimulateTrajectories : %s -> %s", *Source->Name.ToString(), *Destination->Name.ToString());

	int64 Cycles = FPlatformTime::Cycles64();

	// Get the game state
	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	Reset();

	// Run trajectory calculations over a range of altitudes
	const FNovaTrajectoryParameters& Parameters = OrbitalSimulation->MakeTrajectoryParameters(Source, Destination, 1);
	SimulatedTrajectories.Reserve((Slider->GetMaxValue() - Slider->GetMinValue()) / AltitudeStep + 1);
	for (float Altitude = Slider->GetMinValue(); Altitude <= Slider->GetMaxValue(); Altitude += AltitudeStep)
	{
		TSharedPtr<FNovaTrajectory> Trajectory = OrbitalSimulation->ComputeTrajectory(Parameters, Altitude);
		NCHECK(Trajectory.IsValid());
		SimulatedTrajectories.Add(Altitude, Trajectory);
	}

	// Pre-process the trajectory data
	for (const TPair<float, TSharedPtr<FNovaTrajectory>>& AltitudeAndTrajectory : SimulatedTrajectories)
	{
		float                              Altitude   = AltitudeAndTrajectory.Key;
		const TSharedPtr<FNovaTrajectory>& Trajectory = AltitudeAndTrajectory.Value;
		NCHECK(Trajectory.IsValid());

		if (FMath::IsFinite(Trajectory->TotalDeltaV) && FMath::IsFinite(Trajectory->TotalTransferDuration))
		{
			if (Trajectory->TotalDeltaV < MinDeltaV)
			{
				MinDeltaV         = Trajectory->TotalDeltaV;
				MinDeltaVAltitude = Altitude;
			}
			if (Trajectory->TotalDeltaV > MaxDeltaV)
			{
				MaxDeltaV = Trajectory->TotalDeltaV;
			}
			if (Trajectory->TotalTransferDuration < MinDuration)
			{
				MinDuration         = Trajectory->TotalTransferDuration;
				MinDurationAltitude = Altitude;
			}
			if (Trajectory->TotalTransferDuration > MaxDuration)
			{
				MaxDuration = Trajectory->TotalTransferDuration;
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

FText SNovaTrajectoryCalculator::GetDeltaVText() const
{
	const TSharedPtr<FNovaTrajectory>* TrajectoryPtr = SimulatedTrajectories.Find(CurrentAltitude);
	if (TrajectoryPtr)
	{
		const TSharedPtr<FNovaTrajectory>& Trajectory = *TrajectoryPtr;
		NCHECK(Trajectory.IsValid());

		FNumberFormattingOptions NumberOptions;
		NumberOptions.SetMaximumFractionalDigits(1);

		return FText::FormatNamed(
			LOCTEXT("DeltaVFormat", "{deltav} m/s"), TEXT("deltav"), FText::AsNumber(Trajectory->TotalDeltaV, &NumberOptions));
	}

	return FText();
}

FText SNovaTrajectoryCalculator::GetDurationText() const
{
	const TSharedPtr<FNovaTrajectory>* TrajectoryPtr = SimulatedTrajectories.Find(CurrentAltitude);
	if (TrajectoryPtr)
	{
		const TSharedPtr<FNovaTrajectory>& Trajectory = *TrajectoryPtr;
		NCHECK(Trajectory.IsValid());

		return ::GetDurationText(Trajectory->TotalTransferDuration, 2);
	}

	return FText();
}

void SNovaTrajectoryCalculator::OnAltitudeSliderChanged(float Altitude)
{
	CurrentAltitude =
		Slider->GetMinValue() + FMath::RoundToInt((Altitude - Slider->GetMinValue()) / static_cast<float>(AltitudeStep)) * AltitudeStep;

	const TSharedPtr<FNovaTrajectory>* TrajectoryPtr = SimulatedTrajectories.Find(CurrentAltitude);
	if (TrajectoryPtr)
	{
		const TSharedPtr<FNovaTrajectory>& Trajectory = *TrajectoryPtr;
		NCHECK(Trajectory.IsValid());

		OnTrajectoryChanged.ExecuteIfBound(Trajectory);
	}
}

#undef LOCTEXT_NAMESPACE
