// Nova project - Gwennaël Arbona

#include "NovaOrbitalMap.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Nova.h"

#include "Slate/SRetainerWidget.h"

#define LOCTEXT_NAMESPACE "SNovaOrbitalMap"

/*----------------------------------------------------
    Internal structures
----------------------------------------------------*/

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

void SNovaOrbitalMap::Construct(const FArguments& InArgs)
{
	MenuManager               = InArgs._MenuManager;
	TrajectoryPreviewDuration = 2.0f;
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void SNovaOrbitalMap::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Reset state
	BatchedSplines.Empty();
	BatchedPoints.Empty();

	// Process the planet
	FVector2D                Origin = FVector2D(500, 500);
	const class UNovaPlanet* DefaultPlanet =
		MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaPlanet>(FGuid("{0619238A-4DD1-E28B-5F86-A49734CEF648}"));
	AddPlanet(Origin, DefaultPlanet);

	// Get trajectory data
	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	const auto Trajectories = OrbitalSimulation->GetTrajectories();
	const auto Positions    = OrbitalSimulation->GetPositions();

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
	for (const auto ObjectAndTrajectory : Trajectories)
	{
		FNovaMergedOrbit* ExistingTrajectory = MergedOrbits.FindByPredicate(
			[&](const FNovaMergedOrbit& OtherOrbit)
			{
				const FNovaOrbit& Orbit = ObjectAndTrajectory.Value.CurrentOrbit;

				return OtherOrbit.StartAltitude == Orbit.StartAltitude && OtherOrbit.OppositeAltitude == Orbit.OppositeAltitude &&
					   OtherOrbit.StartPhase == Orbit.StartPhase && OtherOrbit.EndPhase == Orbit.EndPhase;
			});

		FNovaOrbitalObject Point = FNovaOrbitalObject(ObjectAndTrajectory.Key, Positions[ObjectAndTrajectory.Key]);

		if (ExistingTrajectory)
		{
			ExistingTrajectory->Objects.Add(Point);
		}
		else
		{
			MergedOrbits.Add(FNovaMergedOrbit(ObjectAndTrajectory.Value.CurrentOrbit, Point));
		}
	}

	// Draw merged orbits
	for (FNovaMergedOrbit& Orbit : MergedOrbits)
	{
		AddOrbit(Origin, Orbit, Orbit.Objects, FNovaSplineStyle(FLinearColor::White));
	}

	// Process preview trajectory
	CurrentPreviewProgress += DeltaTime / TrajectoryPreviewDuration;
	CurrentPreviewProgress = FMath::Min(CurrentPreviewProgress, 1.0f);
	if (CurrentPreviewTrajectory.IsValid())
	{
		AddTrajectory(Origin, *CurrentPreviewTrajectory, FNovaSplineStyle(FLinearColor::Red), false, CurrentPreviewProgress);
	}
}

int32 SNovaOrbitalMap::OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Draw batched brushes
	for (const FNovaBatchedBrush& Brush : BatchedBrushes)
	{
		NCHECK(Brush.Brush);
		FVector2D BrushSize = Brush.Brush->GetImageSize();

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(Brush.Pos - BrushSize / 2, BrushSize, Brush.Scale), Brush.Brush, ESlateDrawEffect::None,
			FLinearColor::White);
	}

	// Draw batched splines
	for (const FNovaBatchedSpline& Spline : BatchedSplines)
	{
		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Spline.P0, Spline.P1,
			Spline.P2, Spline.P3, Spline.WidthOuter * AllottedGeometry.Scale, ESlateDrawEffect::None, Spline.ColorOuter);

		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Spline.P0, Spline.P1,
			Spline.P2, Spline.P3, Spline.WidthInner * AllottedGeometry.Scale, ESlateDrawEffect::None, Spline.ColorInner);
	}

	// Draw batched points
	for (const FNovaBatchedPoint& Point : BatchedPoints)
	{
		FSlateColorBrush WhiteBox          = FSlateColorBrush(FLinearColor::White);
		const FVector2D  BezierPointRadius = FVector2D(Point.Radius, Point.Radius);

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(2 * BezierPointRadius, FSlateLayoutTransform(Point.Pos - BezierPointRadius)), &WhiteBox,
			ESlateDrawEffect::None, Point.Color);
	}

	return SCompoundWidget::OnPaint(PaintArgs, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

FVector2D SNovaOrbitalMap::ComputeDesiredSize(float Scale) const
{
	return FVector2D(1000, 1000);
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaOrbitalMap::PreviewTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory, bool Immediate)
{
	CurrentPreviewTrajectory = Trajectory;
	CurrentPreviewProgress   = Immediate ? TrajectoryPreviewDuration : 0;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void SNovaOrbitalMap::AddPlanet(const FVector2D& Pos, const class UNovaPlanet* Planet)
{
	FNovaBatchedBrush Brush;

	Brush.Brush = &Planet->Image;
	Brush.Pos   = Pos;
	Brush.Scale = 1.0f;

	BatchedBrushes.Add(Brush);
}

void SNovaOrbitalMap::AddTrajectory(
	const FVector2D& Origin, const FNovaTrajectory& Trajectory, const FNovaSplineStyle& Style, bool IncludeCurrent, float Progress)
{
	if (IncludeCurrent)
	{
		TArray<FNovaOrbitalObject> Empty;
		AddOrbit(Origin, Trajectory.CurrentOrbit, Empty, Style);
	}

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

		AddOrbit(Origin, Orbit, Maneuvers, Style);

		if (SkipRemainingTransfers)
		{
			break;
		}
	}
}

TPair<FVector2D, FVector2D> SNovaOrbitalMap::AddOrbit(
	const FVector2D& Origin, const FNovaOrbit& Orbit, const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
{
	const float  RadiusA       = Orbit.StartAltitude;
	const float  RadiusB       = Orbit.OppositeAltitude;
	const float  MajorAxis     = 0.5f * (RadiusA + RadiusB);
	const float  MinorAxis     = FMath::Sqrt(RadiusA * RadiusB);
	const float& Phase         = Orbit.StartPhase;
	const float& InitialAngle  = 0;
	const float  AngularLength = Orbit.EndPhase - Orbit.StartPhase;
	const float  Offset        = 0.5f * (RadiusB - RadiusA);

	return AddOrbitInternal(FNovaSplineOrbit(Origin, MajorAxis, MinorAxis, Phase, InitialAngle, AngularLength, Offset), Objects, Style);
}

void SNovaOrbitalMap::AddOrbitalObject(const FNovaOrbitalObject& Object, const FLinearColor& Color)
{
	FNovaBatchedPoint Point;
	Point.Pos    = Object.Position;
	Point.Color  = Color;
	Point.Radius = 4.0f;
	Point.Object = Object;

	BatchedPoints.Add(Point);
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

		// Process points of interest
		for (FNovaOrbitalObject& Point : Objects)
		{
			const float ScaledPoint = 4.0f * (Point.Phase / 360.0f);
			if (Point.Phase >= Orbit.InitialAngle && Point.Phase <= Orbit.AngularLength && ScaledPoint >= SplineIndex &&
				ScaledPoint < SplineIndex + 1)
			{
				const float Alpha = FMath::Fmod(Point.Phase, 90.0f) / 90.0f;
				Point.Position    = DeCasteljauInterp(P0, P1, P2, P3, Alpha);
			}
		}

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
		BatchedSplines.Add(Spline);

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
		AddOrbitalObject(Object, Style.ColorInner);
	}

	return TPair<FVector2D, FVector2D>(InitialPosition, FinalPosition);
}

void SNovaOrbitalMap::AddTestOrbits()
{
	FVector2D Origin = FVector2D(500, 500);

	auto AddCircularOrbit =
		[&](const FVector2D& Origin, float Radius, const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
	{
		AddOrbitInternal(FNovaSplineOrbit(Origin, Radius), Objects, Style);
	};

	auto AddPartialCircularOrbit = [&](const FVector2D& Origin, float Radius, float Phase, float InitialAngle, float AngularLength,
									   const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
	{
		AddOrbitInternal(FNovaSplineOrbit(Origin, Radius, Radius, Phase, InitialAngle, AngularLength, 0.0f), Objects, Style);
	};

	auto AddTransferOrbit = [&](const FVector2D& Origin, float RadiusA, float RadiusB, float Phase, float InitialAngle,
								const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
	{
		float MajorAxis = 0.5f * (RadiusA + RadiusB);
		float MinorAxis = FMath::Sqrt(RadiusA * RadiusB);

		TPair<FVector2D, FVector2D> InitialAndFinalPosition = AddOrbitInternal(
			FNovaSplineOrbit(Origin, MajorAxis, MinorAxis, Phase, InitialAngle, 180, 0.5f * (RadiusB - RadiusA)), Objects, Style);
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

#undef LOCTEXT_NAMESPACE
