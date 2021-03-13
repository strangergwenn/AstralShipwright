// Nova project - Gwennaël Arbona

#include "NovaOrbitalMap.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaOrbitalMap"

/*----------------------------------------------------
    Internal structures
----------------------------------------------------*/

FText FNovaOrbitalObject::GetText(double CurrentTime) const
{
	if (Area)
	{
		return Area->Name;
	}
	else if (Spacecraft.IsValid())
	{
		FString IDentifier = Spacecraft->Identifier.ToString(EGuidFormats::DigitsWithHyphens);
		int32   Index;
		if (IDentifier.FindLastChar('-', Index))
		{
			return FText::FromString(IDentifier.RightChop(Index));
		}
		else
		{
			return FText();
		}
	}
	else if (Maneuver)
	{
		FNumberFormattingOptions NumberOptions;
		NumberOptions.SetMaximumFractionalDigits(1);

		return FText::FormatNamed(LOCTEXT("ManeuverFormat", "{deltav} m/s maneuver at {phase}° in {time}"), TEXT("phase"),
			FText::AsNumber(FMath::Fmod(Maneuver->Phase, 360.0f), &NumberOptions), TEXT("time"),
			GetDurationText(Maneuver->Time - CurrentTime), TEXT("deltav"), FText::AsNumber(Maneuver->DeltaV, &NumberOptions));
	}
	else
	{
		return FText();
	}
}

/** Geometry of an orbit on the map */
struct FNovaSplineOrbit
{
	FNovaSplineOrbit(const FVector2D& Orig, float R)
		: Origin(Orig), Width(R), Height(R), Phase(0), InitialAngle(0), AngularLength(360), OriginOffset(0)
	{}

	FNovaSplineOrbit(const FVector2D& Orig, float W, float H, float P) : FNovaSplineOrbit(Orig, W)
	{
		Height = H;
		Phase  = P;
	}

	FNovaSplineOrbit(const FVector2D& Orig, float W, float H, float P, float Initial, float Length, float Offset)
		: FNovaSplineOrbit(Orig, W, H, P)
	{
		InitialAngle  = Initial;
		AngularLength = Length;
		OriginOffset  = Offset;
	}

	FVector2D Origin;
	float     Width;
	float     Height;
	float     Phase;
	float     InitialAngle;
	float     AngularLength;
	float     OriginOffset;
};

/** Orbit drawing style */
struct FNovaSplineStyle
{
	FNovaSplineStyle() : ColorInner(FLinearColor::White), ColorOuter(FLinearColor::Black), WidthInner(1.0f), WidthOuter(2.0f)
	{}

	FNovaSplineStyle(const FLinearColor& Color) : FNovaSplineStyle()
	{
		ColorInner = Color;
		ColorOuter = Color;
	}

	FLinearColor ColorInner;
	FLinearColor ColorOuter;
	float        WidthInner;
	float        WidthOuter;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaOrbitalMap::SNovaOrbitalMap() : CurrentPreviewProgress(0), CurrentDesiredSize(1000.0f), CurrentDrawScale(1.0f), CurrentZoomSpeed(0.0f)
{}

void SNovaOrbitalMap::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Settings
	MenuManager                = InArgs._MenuManager;
	TrajectoryPreviewDuration  = 2.0f;
	TrajectoryZoomSpeed        = 0.5f;
	TrajectoryZoomAcceleration = 1.0f;
	TrajectoryZoomSnappinness  = 10.0f;
	TrajectoryInflationRatio   = 1.2f;

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Bottom)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(Theme.BlurRadius)
			.BlurStrength(Theme.BlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.Padding(Theme.ContentPadding)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SNovaOrbitalMap::GetHoverText)
					.TextStyle(&Theme.MainFont)
				]
			]
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaOrbitalMap::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Debug data
	FVector2D                Origin = FVector2D(0, 0);
	const class UNovaPlanet* DefaultPlanet =
		MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaPlanet>(FGuid("{0619238A-4DD1-E28B-5F86-A49734CEF648}"));

	// Reset state
	ClearBatches();
	CurrentDesiredSize = 100;
	DesiredObjectTexts.Empty();
	CurrentOrigin = GetTickSpaceGeometry().GetLocalSize() / 2;

	// Run processes
	AddPlanet(Origin, DefaultPlanet);
	ProcessAreas(Origin);
	ProcessTrajectoryPreview(Origin, DeltaTime);
	ProcessDrawScale(DeltaTime);
}

void SNovaOrbitalMap::PreviewTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory, bool Immediate)
{
	CurrentPreviewTrajectory = Trajectory;
	CurrentPreviewProgress   = Immediate ? TrajectoryPreviewDuration : 0;
}

/*----------------------------------------------------
    Slate callbacks
----------------------------------------------------*/

FText SNovaOrbitalMap::GetHoverText() const
{
	FString Result;

	for (const FString& Text : DesiredObjectTexts)
	{
		if (Result.Len())
		{
			Result += '\n';
		}
		Result += Text;
	}

	return FText::FromString(Result);
}

/*----------------------------------------------------
    High-level internals
----------------------------------------------------*/

void SNovaOrbitalMap::ProcessAreas(const FVector2D& Origin)
{
	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);

	// Orbit merging structure
	struct FNovaMergedOrbit : FNovaOrbit
	{
		FNovaMergedOrbit(const FNovaOrbit& Base, const FNovaOrbitalObject& Point) : FNovaOrbit(Base)
		{
			Objects.Add(Point);
		}

		TArray<FNovaOrbitalObject> Objects;
	};
	TArray<FNovaMergedOrbit> MergedOrbits;

	// Merge orbits
	for (const auto AreaAndOrbitAndPosition : OrbitalSimulation->GetAreasOrbitAndPosition())
	{
		const TPair<FNovaOrbit, double>& OrbitAndPosition = AreaAndOrbitAndPosition.Value;
		const FNovaOrbit&                Orbit            = OrbitAndPosition.Key;
		const float                      Phase            = OrbitAndPosition.Value;

		CurrentDesiredSize = FMath::Max(CurrentDesiredSize, Orbit.GetHighestAltitude());

		FNovaMergedOrbit* ExistingTrajectory = MergedOrbits.FindByPredicate(
			[&](const FNovaMergedOrbit& OtherOrbit)
			{
				return OtherOrbit.StartAltitude == Orbit.StartAltitude && OtherOrbit.OppositeAltitude == Orbit.OppositeAltitude &&
					   OtherOrbit.StartPhase == Orbit.StartPhase && OtherOrbit.EndPhase == Orbit.EndPhase;
			});

		FNovaOrbitalObject Point = FNovaOrbitalObject(AreaAndOrbitAndPosition.Key, Phase);

		if (ExistingTrajectory)
		{
			ExistingTrajectory->Objects.Add(Point);
		}
		else
		{
			MergedOrbits.Add(FNovaMergedOrbit(Orbit, Point));
		}
	}

	// Draw merged orbits
	for (FNovaMergedOrbit& Orbit : MergedOrbits)
	{
		AddOrbit(Origin, Orbit, Orbit.Objects, FNovaSplineStyle(FLinearColor::White));
	}
}

void SNovaOrbitalMap::ProcessTrajectoryPreview(const FVector2D& Origin, float DeltaTime)
{
	CurrentPreviewProgress += DeltaTime / TrajectoryPreviewDuration;
	CurrentPreviewProgress = FMath::Min(CurrentPreviewProgress, 1.0f);

	if (CurrentPreviewTrajectory.IsValid())
	{
		AddTrajectory(Origin, *CurrentPreviewTrajectory, FNovaSplineStyle(FLinearColor::Red), CurrentPreviewProgress);
		CurrentDesiredSize = FMath::Max(CurrentDesiredSize, CurrentPreviewTrajectory->GetHighestAltitude());
	};
}

void SNovaOrbitalMap::ProcessDrawScale(float DeltaTime)
{
	CurrentDesiredSize *= 2 * TrajectoryInflationRatio;

	const float CurrentDesiredScale =
		FMath::Min(GetTickSpaceGeometry().GetLocalSize().X, GetTickSpaceGeometry().GetLocalSize().Y) / CurrentDesiredSize;
	float ScaleDelta        = (CurrentDesiredScale - CurrentDrawScale) * TrajectoryZoomSnappinness;
	float ScaleAcceleration = CurrentZoomSpeed - ScaleDelta;
	ScaleAcceleration       = FMath::Clamp(ScaleAcceleration, -TrajectoryZoomAcceleration, TrajectoryZoomAcceleration);

	CurrentZoomSpeed += ScaleAcceleration * DeltaTime;
	CurrentZoomSpeed = FMath::Clamp(CurrentZoomSpeed, -TrajectoryZoomSpeed, TrajectoryZoomSpeed);
	CurrentDrawScale += ScaleDelta * DeltaTime;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void SNovaOrbitalMap::AddPlanet(const FVector2D& Pos, const class UNovaPlanet* Planet)
{
	NCHECK(Planet);

	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Planet brush
	FNovaBatchedBrush Brush;
	Brush.Brush = &Planet->Image;
	Brush.Pos   = Pos * CurrentDrawScale;
	BatchedBrushes.AddUnique(Brush);

	FVector2D BrushSize = Brush.Brush->GetImageSize();

	// Add text label
	FNovaBatchedText Text;
	Text.Text      = Planet->Name.ToUpper();
	Text.Pos       = Brush.Pos - FVector2D(0, Planet->Image.GetImageSize().Y);
	Text.TextStyle = &Theme.SubtitleFont;
	BatchedTexts.Add(Text);
}

void SNovaOrbitalMap::AddTrajectory(
	const FVector2D& Position, const FNovaTrajectory& Trajectory, const FNovaSplineStyle& Style, float Progress)
{
	for (int32 TransferIndex = 0; TransferIndex < Trajectory.TransferOrbits.Num(); TransferIndex++)
	{
		const TArray<FNovaOrbit>& TransferOrbits = Trajectory.TransferOrbits;
		FNovaOrbit                Orbit          = TransferOrbits[TransferIndex];

		bool  SkipRemainingTransfers = false;
		float CurrentPhase = FMath::Lerp(TransferOrbits[0].StartPhase, TransferOrbits[TransferOrbits.Num() - 1].EndPhase, Progress);
		if (CurrentPhase < Orbit.EndPhase)
		{
			Orbit.EndPhase         = CurrentPhase;
			SkipRemainingTransfers = true;
		}

		TArray<FNovaOrbitalObject> Maneuvers;
		for (const FNovaManeuver& Maneuver : Trajectory.Maneuvers)
		{
			Maneuvers.Add(Maneuver);
		}

		AddOrbit(Position, Orbit, Maneuvers, Style);

		if (SkipRemainingTransfers)
		{
			break;
		}
	}
}

TPair<FVector2D, FVector2D> SNovaOrbitalMap::AddOrbit(
	const FVector2D& Position, const FNovaOrbit& Orbit, const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
{
	const FVector2D LocalPosition = Position * CurrentDrawScale;

	const float  RadiusA       = CurrentDrawScale * Orbit.StartAltitude;
	const float  RadiusB       = CurrentDrawScale * Orbit.OppositeAltitude;
	const float  MajorAxis     = 0.5f * (RadiusA + RadiusB);
	const float  MinorAxis     = FMath::Sqrt(RadiusA * RadiusB);
	const float& Phase         = Orbit.StartPhase;
	const float& InitialAngle  = 0;
	const float  AngularLength = Orbit.EndPhase - Orbit.StartPhase;
	const float  Offset        = 0.5f * (RadiusB - RadiusA);

	return AddOrbitInternal(
		FNovaSplineOrbit(LocalPosition, MajorAxis, MinorAxis, Phase, InitialAngle, AngularLength, Offset), Objects, Style);
}

void SNovaOrbitalMap::AddOrbitalObject(const FNovaOrbitalObject& Object, const FLinearColor& Color)
{
	bool IsHovered =
		(CurrentOrigin + Object.Position - GetTickSpaceGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos())).Size() < 50;

	FNovaBatchedPoint Point;
	Point.Pos    = Object.Position;
	Point.Color  = Color;
	Point.Radius = IsHovered ? 8.0f : 4.0f;

	BatchedPoints.AddUnique(Point);

	if (IsHovered)
	{
		ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
		NCHECK(GameState);
		ANovaGameWorld* GameWorld = GameState->GetGameWorld();
		NCHECK(GameWorld);
		UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
		NCHECK(OrbitalSimulation);

		DesiredObjectTexts.AddUnique(Object.GetText(OrbitalSimulation->GetCurrentTime()).ToString());
	}
}

TPair<FVector2D, FVector2D> SNovaOrbitalMap::AddOrbitInternal(
	const FNovaSplineOrbit& Orbit, TArray<FNovaOrbitalObject> Objects, const FNovaSplineStyle& Style)
{
	bool      FirstRenderedSpline = true;
	FVector2D InitialPosition     = FVector2D::ZeroVector;
	FVector2D FinalPosition       = FVector2D::ZeroVector;

	for (int32 SplineIndex = 0; SplineIndex < 4; SplineIndex++)
	{
		// Useful constants
		const float     CurrentStartAngle = SplineIndex * 90.0f;
		const float     CurrentEndAngle   = CurrentStartAngle + 90.0f;
		constexpr float CircleRatio       = 0.55228475f;

		// Define the control points for a 90° segment of the orbit
		FVector2D   BezierPoints[4];
		const float FirstAxis  = SplineIndex % 2 == 0 ? Orbit.Width : Orbit.Height;
		const float SecondAxis = SplineIndex % 2 == 0 ? Orbit.Height : Orbit.Width;
		BezierPoints[0]        = FVector2D(FirstAxis, 0.5f);
		BezierPoints[3]        = FVector2D(-0.5f, -SecondAxis);
		BezierPoints[1]        = BezierPoints[0] - FVector2D(0, CircleRatio * SecondAxis);
		BezierPoints[2]        = BezierPoints[3] + FVector2D(CircleRatio * FirstAxis, 0);

		// Rotate all segments around the orbit
		auto Transform = [&Orbit, SplineIndex](const FVector2D& ControlPoint)
		{
			return Orbit.Origin + FVector2D(-Orbit.OriginOffset, 0).GetRotated(-Orbit.Phase) +
				   ControlPoint.GetRotated(-Orbit.Phase - SplineIndex * 90.0f);
		};
		FVector2D P0 = Transform(BezierPoints[0]);
		FVector2D P1 = Transform(BezierPoints[1]);
		FVector2D P2 = Transform(BezierPoints[2]);
		FVector2D P3 = Transform(BezierPoints[3]);

		// Split the curve to account for initial angle
		float CurrentSegmentLength = 90.0f;
		if (Orbit.InitialAngle >= CurrentEndAngle)
		{
			continue;
		}
		else if (Orbit.InitialAngle > CurrentStartAngle)
		{
			const float       RelativeInitialAngle = FMath::Fmod(Orbit.InitialAngle, 90.0f);
			TArray<FVector2D> ControlPoints        = DeCasteljauSplit(P0, P1, P2, P3, RelativeInitialAngle / 90.0f);
			CurrentSegmentLength                   = 90.0f - RelativeInitialAngle;

			P0 = ControlPoints[3];
			P1 = ControlPoints[4];
			P2 = ControlPoints[5];
			P3 = ControlPoints[6];
		}

		// Split the curve to account for angular length
		if (Orbit.AngularLength <= CurrentStartAngle)
		{
			break;
		}
		if (Orbit.AngularLength < CurrentEndAngle)
		{
			float             Alpha         = FMath::Fmod(Orbit.AngularLength, 90.0f) / CurrentSegmentLength;
			TArray<FVector2D> ControlPoints = DeCasteljauSplit(P0, P1, P2, P3, Alpha);

			P0 = ControlPoints[0];
			P1 = ControlPoints[1];
			P2 = ControlPoints[2];
			P3 = ControlPoints[3];
		}

		// Process points of interest
		for (FNovaOrbitalObject& Object : Objects)
		{
			const float CurrentSplineStartPhase = Orbit.Phase + CurrentStartAngle;
			float       CurrentSplineEndPhase   = Orbit.Phase + CurrentEndAngle;

			if (!Object.Valid && Object.Phase >= CurrentSplineStartPhase && Object.Phase <= CurrentSplineEndPhase)
			{
				float Alpha = FMath::Fmod(Object.Phase - Orbit.Phase, 90.0f) / 90.0f;
				if (Alpha == 0 && Object.Phase != CurrentSplineStartPhase)
				{
					Alpha = 1.0f;
				}

				Object.Position = DeCasteljauInterp(P0, P1, P2, P3, Alpha);
				Object.Valid    = true;
			}
		}

		// Batch the spline segment
		FNovaBatchedSpline Spline;
		Spline.P0         = P0;
		Spline.P1         = P1;
		Spline.P2         = P2;
		Spline.P3         = P3;
		Spline.ColorInner = Style.ColorInner;
		Spline.ColorOuter = Style.ColorOuter;
		Spline.WidthInner = Style.WidthInner;
		Spline.WidthOuter = Style.WidthOuter;
		BatchedSplines.AddUnique(Spline);

		// Report the initial and final positions
		if (FirstRenderedSpline)
		{
			InitialPosition     = P0;
			FirstRenderedSpline = false;
		}
		FinalPosition = P3;
	}

	// Draw positioned objects
	for (const FNovaOrbitalObject& Object : Objects)
	{
		if (Object.Valid)
		{
			AddOrbitalObject(Object, Style.ColorInner);
		}
	}

	return TPair<FVector2D, FVector2D>(InitialPosition, FinalPosition);
}

void SNovaOrbitalMap::AddTestOrbits()
{
	FVector2D Origin = FVector2D(500, 500);

	auto AddCircularOrbit =
		[&](const FVector2D& Position, float Radius, const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
	{
		AddOrbitInternal(FNovaSplineOrbit(Position, Radius), Objects, Style);
	};

	auto AddPartialCircularOrbit = [&](const FVector2D& Position, float Radius, float Phase, float InitialAngle, float AngularLength,
									   const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
	{
		AddOrbitInternal(FNovaSplineOrbit(Position, Radius, Radius, Phase, InitialAngle, AngularLength, 0.0f), Objects, Style);
	};

	auto AddTransferOrbit = [&](const FVector2D& Position, float RadiusA, float RadiusB, float Phase, float InitialAngle,
								const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
	{
		float MajorAxis = 0.5f * (RadiusA + RadiusB);
		float MinorAxis = FMath::Sqrt(RadiusA * RadiusB);

		TPair<FVector2D, FVector2D> InitialAndFinalPosition = AddOrbitInternal(
			FNovaSplineOrbit(Position, MajorAxis, MinorAxis, Phase, InitialAngle, 180, 0.5f * (RadiusB - RadiusA)), Objects, Style);
	};

	TArray<FNovaOrbitalObject> Objects;

	AddCircularOrbit(Origin, 300, Objects, FNovaSplineStyle(FLinearColor::White));
	AddCircularOrbit(Origin, 450, Objects, FNovaSplineStyle(FLinearColor::White));
	AddTransferOrbit(Origin, 300, 450, 45, 0, Objects, FNovaSplineStyle(FLinearColor::Red));

	AddTransferOrbit(Origin, 300, 500, 90, 0, Objects, FNovaSplineStyle(FLinearColor::Green));
	AddTransferOrbit(Origin, 500, 450, 180 + 90, 0, Objects, FNovaSplineStyle(FLinearColor::Green));

	AddTransferOrbit(Origin, 300, 250, 135, 0, Objects, FNovaSplineStyle(FLinearColor::Blue));
	AddPartialCircularOrbit(Origin, 250, 135 + 180, 0, 45, Objects, FNovaSplineStyle(FLinearColor::Blue));
	AddTransferOrbit(Origin, 250, 450, 135 + 180 + 45, 0, Objects, FNovaSplineStyle(FLinearColor::Blue));
}

/*----------------------------------------------------
       Batch renderer
----------------------------------------------------*/

void SNovaOrbitalMap::ClearBatches()
{
	BatchedSplines.Empty();
	BatchedPoints.Empty();
	BatchedBrushes.Empty();
	BatchedTexts.Empty();
}

int32 SNovaOrbitalMap::OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
#if 0
	NDIS("Painting %d brushes, %d splines, %d points, %d texts", BatchedBrushes.Num(), BatchedSplines.Num(), BatchedPoints.Num(),
		BatchedTexts.Num());
#endif

	// Draw batched brushes
	for (const FNovaBatchedBrush& Brush : BatchedBrushes)
	{
		NCHECK(Brush.Brush);
		FVector2D BrushSize = Brush.Brush->GetImageSize();

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(BrushSize, FSlateLayoutTransform(CurrentOrigin + Brush.Pos - BrushSize / 2)), Brush.Brush,
			ESlateDrawEffect::None, FLinearColor::White);
	}

	// Draw batched splines
	for (const FNovaBatchedSpline& Spline : BatchedSplines)
	{
		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CurrentOrigin + Spline.P0,
			CurrentOrigin + Spline.P1, CurrentOrigin + Spline.P2, CurrentOrigin + Spline.P3, Spline.WidthOuter * AllottedGeometry.Scale,
			ESlateDrawEffect::None, Spline.ColorOuter);

		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CurrentOrigin + Spline.P0,
			CurrentOrigin + Spline.P1, CurrentOrigin + Spline.P2, CurrentOrigin + Spline.P3, Spline.WidthInner * AllottedGeometry.Scale,
			ESlateDrawEffect::None, Spline.ColorInner);
	}

	// Draw batched points
	for (const FNovaBatchedPoint& Point : BatchedPoints)
	{
		FSlateColorBrush WhiteBox          = FSlateColorBrush(FLinearColor::White);
		const FVector2D  BezierPointRadius = FVector2D(Point.Radius, Point.Radius);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(2 * BezierPointRadius, FSlateLayoutTransform(CurrentOrigin + Point.Pos - BezierPointRadius)),
			&WhiteBox, ESlateDrawEffect::NoPixelSnapping, Point.Color);
	}

	// Draw batched texts
	for (const FNovaBatchedText& Text : BatchedTexts)
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		FVector2D                           TextSize           = FontMeasureService->Measure(Text.Text, Text.TextStyle->Font);

		FPaintGeometry TextGeometry =
			AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(CurrentOrigin + Text.Pos - TextSize / 2));

		FSlateDrawElement::MakeText(OutDrawElements, LayerId, TextGeometry, Text.Text, Text.TextStyle->Font);
	}

	return SCompoundWidget::OnPaint(PaintArgs, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

#undef LOCTEXT_NAMESPACE
