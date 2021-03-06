// Nova project - Gwennaël Arbona

#include "NovaTrajectoryCalculator.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaSlider.h"

#include "Nova/Nova.h"

#include "Widgets/Colors/SComplexGradient.h"

#define LOCTEXT_NAMESPACE "SNovaTrajectoryCalculator"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaTrajectoryCalculator::Construct(const FArguments& InArgs)
{
	// Get data
	const FNovaMainTheme&   Theme           = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme     = FNovaStyleSet::GetButtonTheme();
	const FNovaSliderTheme& SliderTheme     = FNovaStyleSet::GetTheme<FNovaSliderTheme>("DefaultSlider");
	const FSlateBrush*      BackgroundBrush = FNovaStyleSet::GetBrush("Game/SB_TrajectoryCalculator");
	MenuManager                             = InArgs._MenuManager;
	CurrentAlpha                            = InArgs._CurrentAlpha;
	OnAltitudeChanged                       = InArgs._OnAltitudeChanged;

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
					SNew(SNovaButton) // No navigation
					.Text(LOCTEXT("MinimizeDeltaV", "Minimize Delta-V"))
					.HelpText(LOCTEXT("MinimizeDeltaVHelp", "Configure the trajectory to minimize the Delta-V cost"))
					.Focusable(false)
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
					SNew(SNovaButton) // No navigation
					.Text(LOCTEXT("MinimizeTravelTime", "Minimize travel time"))
					.HelpText(LOCTEXT("MinimizeTravelTimeHelp", "Configure the trajectory to minimize the travel tile"))
					.Focusable(false)
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
			.OnValueChanged(InArgs._OnAltitudeChanged)
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
			for (const FNovaTrajectoryCharacteristics& Res : SimulatedTrajectoriesCharacteristics)
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
	SimulatedTrajectoriesCharacteristics = {};
	TrajectoryDeltaVGradientData         = {FLinearColor::Black, FLinearColor::Black};
	TrajectoryDurationGradientData       = {FLinearColor::Black, FLinearColor::Black};

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

	// Get the game state
	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	Reset();

	// Run trajectory calculations over a range of altitudes
	for (float Altitude = Slider->GetMinValue(); Altitude <= Slider->GetMaxValue(); Altitude += 5)
	{
		TSharedPtr<FNovaTrajectory> Trajectory = OrbitalSimulation->ComputeTrajectory(Source, Destination, Altitude);

		FNovaTrajectoryCharacteristics TrajectoryCharacteristics;
		TrajectoryCharacteristics.IntermediateAltitude = Altitude;
		TrajectoryCharacteristics.Duration             = Trajectory->TotalTransferDuration;
		TrajectoryCharacteristics.DeltaV               = Trajectory->TotalDeltaV;
		SimulatedTrajectoriesCharacteristics.Add(TrajectoryCharacteristics);
	}

	// Pre-process the trajectory data
	for (FNovaTrajectoryCharacteristics& Characteristics : SimulatedTrajectoriesCharacteristics)
	{
		Characteristics.DeltaV   = FMath::LogX(10, Characteristics.DeltaV);
		Characteristics.Duration = FMath::LogX(10, Characteristics.Duration);

		if (Characteristics.DeltaV < MinDeltaV)
		{
			MinDeltaV                          = Characteristics.DeltaV;
			MinDeltaVTrajectoryCharacteristics = Characteristics;
		}
		if (Characteristics.DeltaV > MaxDeltaV && FMath::IsFinite(Characteristics.DeltaV))
		{
			MaxDeltaV = Characteristics.DeltaV;
		}
		if (Characteristics.Duration < MinDuration)
		{
			MinDuration                          = Characteristics.Duration;
			MinDurationTrajectoryCharacteristics = Characteristics;
		}
		if (Characteristics.Duration > MaxDuration && FMath::IsFinite(Characteristics.Duration))
		{
			MaxDuration = Characteristics.Duration;
		}
	}

	NeedTrajectoryDisplayUpdate = true;
}

void SNovaTrajectoryCalculator::OptimizeForDeltaV()
{
	NLOG("SNovaTrajectoryCalculator::OptimizeForDeltaV");
	Slider->SetCurrentValue(MinDeltaVTrajectoryCharacteristics.IntermediateAltitude);
	OnAltitudeChanged.ExecuteIfBound(MinDeltaVTrajectoryCharacteristics.IntermediateAltitude);
}

void SNovaTrajectoryCalculator::OptimizeForDuration()
{
	NLOG("SNovaTrajectoryCalculator::OptimizeForDuration");
	Slider->SetCurrentValue(MinDurationTrajectoryCharacteristics.IntermediateAltitude);
	OnAltitudeChanged.ExecuteIfBound(MinDurationTrajectoryCharacteristics.IntermediateAltitude);
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

FText SNovaTrajectoryCalculator::GetDeltaVText() const
{
	FNumberFormattingOptions NumberOptions;
	NumberOptions.SetMaximumFractionalDigits(1);

	return FText::FormatNamed(LOCTEXT("DeltaVFormat", "{deltav} m/s"), TEXT("deltav"), FText::AsNumber(42, &NumberOptions));
}

FText SNovaTrajectoryCalculator::GetDurationText() const
{
	FNumberFormattingOptions NumberOptions;
	NumberOptions.SetMaximumFractionalDigits(1);

	return FText::FormatNamed(LOCTEXT("DurationFormat", "{hours} hours"), TEXT("hours"), FText::AsNumber(57.2, &NumberOptions));
}

#undef LOCTEXT_NAMESPACE
