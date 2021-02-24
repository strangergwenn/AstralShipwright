// Nova project - Gwennaël Arbona

#include "NovaOrbitalMap.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Nova.h"

#include "Slate/SRetainerWidget.h"

#define LOCTEXT_NAMESPACE "SNovaOrbitalMap"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaOrbitalMap::Construct(const FArguments& InArgs)
{
	MenuManager = InArgs._MenuManager;
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

	//// DEBUG - get trajectories

	ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(GameState);
	ANovaGameWorld* GameWorld = GameState->GetGameWorld();
	NCHECK(GameWorld);
	UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
	NCHECK(OrbitalSimulation);
	TMap<const class UNovaArea*, FNovaSpacecraftTrajectory> AreaTrajectories = OrbitalSimulation->GetAreaTrajectories();

	//// DEBUG - merge orbits

	struct FNovaMergedSpacecraftTrajectory : FNovaSpacecraftTrajectory
	{
		FNovaMergedSpacecraftTrajectory(const FNovaSpacecraftTrajectory& Base) : FNovaSpacecraftTrajectory(Base)
		{}

		TArray<float> Spacecraft;
	};

	TArray<FNovaMergedSpacecraftTrajectory> MergedTrajectories;
	for (const auto AreaAndTrajectory : AreaTrajectories)
	{
		FNovaMergedSpacecraftTrajectory* ExistingTrajectory = MergedTrajectories.FindByPredicate(
			[&](const FNovaMergedSpacecraftTrajectory& OtherTrajectory)
			{
				return OtherTrajectory.StartAltitude == AreaAndTrajectory.Value.StartAltitude &&
					   OtherTrajectory.EndAltitude == AreaAndTrajectory.Value.EndAltitude &&
					   OtherTrajectory.StartPhase == AreaAndTrajectory.Value.StartPhase &&
					   OtherTrajectory.EndPhase == AreaAndTrajectory.Value.EndPhase;
			});

		if (ExistingTrajectory)
		{
			ExistingTrajectory->Spacecraft.Add(AreaAndTrajectory.Value.CurrentPhase);
		}
		else
		{
			MergedTrajectories.Add(AreaAndTrajectory.Value);
		}
	}

	//// DEBUG - draw

	FNovaSplineStyle Style(FLinearColor::White);
	FVector2D        Origin = FVector2D(500, 500);

	for (const FNovaMergedSpacecraftTrajectory& Trajectory : MergedTrajectories)
	{
		TArray<FVector2D> SpacecraftLocations;

		if (Trajectory.StartAltitude == Trajectory.EndAltitude && Trajectory.StartPhase == 0 && Trajectory.EndPhase == 360)
		{
			SpacecraftLocations = AddCircularOrbit(Origin, Trajectory.StartAltitude, Trajectory.Spacecraft, Style);
		}
		else if (Trajectory.StartAltitude == Trajectory.EndAltitude)
		{
			SpacecraftLocations = AddPartialCircularOrbit(Origin, Trajectory.StartAltitude, Trajectory.StartPhase, 0,
				Trajectory.EndPhase - Trajectory.StartPhase, Trajectory.Spacecraft, Style);
		}
		else
		{
			SpacecraftLocations = AddTransferOrbit(
				Origin, Trajectory.StartAltitude, Trajectory.EndAltitude, Trajectory.StartPhase, 0, Trajectory.Spacecraft, Style);
		}

		for (const FVector2D& SpacecraftLocation : SpacecraftLocations)
		{
			AddPoint(SpacecraftLocation, Style.ColorInner);
		}
	}

#if 0

	DrawCircularOrbit(SlateArgs, Origin, 300, PointsOfInterest, Style);
	DrawCircularOrbit(SlateArgs, Origin, 450, PointsOfInterest, Style);
	DrawTransferOrbit(SlateArgs, Origin, 300, 450, 45, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Red));

	DrawTransferOrbit(SlateArgs, Origin, 300, 500, 90, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Green));
	DrawTransferOrbit(SlateArgs, Origin, 500, 450, 180 + 90, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Green));

	DrawTransferOrbit(SlateArgs, Origin, 300, 250, 135, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));
	DrawPartialCircularOrbit(SlateArgs, Origin, 250, 135 + 180, 0, 45, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));
	DrawTransferOrbit(SlateArgs, Origin, 250, 450, 135 + 180 + 45, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));

#endif
}

int32 SNovaOrbitalMap::OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
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

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

TArray<FVector2D> SNovaOrbitalMap::AddCircularOrbit(
	const FVector2D& Origin, float Radius, const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style)
{
	FNovaSplineResults Results = AddOrbit(FNovaSplineOrbit(Origin, Radius), PointsOfInterest, Style);
	return Results.PointsOfInterest;
}

TArray<FVector2D> SNovaOrbitalMap::AddPartialCircularOrbit(const FVector2D& Origin, float Radius, float Phase, float InitialAngle,
	float AngularLength, const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style)
{
	FNovaSplineResults Results =
		AddOrbit(FNovaSplineOrbit(Origin, Radius, Radius, Phase, InitialAngle, AngularLength, 0.0f), PointsOfInterest, Style);
	return Results.PointsOfInterest;
}

TArray<FVector2D> SNovaOrbitalMap::AddTransferOrbit(const FVector2D& Origin, float RadiusA, float RadiusB, float Phase, float InitialAngle,
	const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style)
{
	float MajorAxis = 0.5f * (RadiusA + RadiusB);
	float MinorAxis = FMath::Sqrt(RadiusA * RadiusB);

	FNovaSplineResults Results = AddOrbit(
		FNovaSplineOrbit(Origin, MajorAxis, MinorAxis, Phase, InitialAngle, 180, 0.5f * (RadiusB - RadiusA)), PointsOfInterest, Style);
	AddPoint(Results.InitialPosition, Style.ColorInner);
	AddPoint(Results.FinalPosition, Style.ColorInner);

	return Results.PointsOfInterest;
}

void SNovaOrbitalMap::AddPoint(const FVector2D& Pos, const FLinearColor& Color, float Radius)
{
	FNovaBatchedPoint Point;
	Point.Pos    = Pos;
	Point.Color  = Color;
	Point.Radius = Radius;

	BatchedPoints.Add(Point);
}

FNovaSplineResults SNovaOrbitalMap::AddOrbit(
	const FNovaSplineOrbit& Orbit, const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style)
{
	bool               FirstRenderedSpline = true;
	FNovaSplineResults Results;

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
		for (float Point : PointsOfInterest)
		{
			const float ScaledPoint = 4.0f * (Point / 360.0f);
			if (Point >= Orbit.InitialAngle && Point <= Orbit.AngularLength && ScaledPoint >= SplineIndex && ScaledPoint < SplineIndex + 1)
			{
				const float Alpha = FMath::Fmod(Point, 90.0f) / 90.0f;
				Results.PointsOfInterest.Add(DeCasteljauInterp(P0, P1, P2, P3, Alpha));
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
			Results.InitialPosition = P0;
			FirstRenderedSpline     = false;
		}
		Results.FinalPosition = P3;
	}

	return Results;
}

#undef LOCTEXT_NAMESPACE
