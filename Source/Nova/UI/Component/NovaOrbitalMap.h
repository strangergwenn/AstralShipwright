// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/Game/NovaGameTypes.h"

#include "Widgets/SCompoundWidget.h"

/*----------------------------------------------------
    Internal structures
----------------------------------------------------*/

/** Point of interest on the map */
struct FNovaOrbitalObject
{
	FNovaOrbitalObject() : Area(nullptr), Spacecraft(nullptr), Maneuver(nullptr)
	{}

	FNovaOrbitalObject(const class UNovaArea* A, float P) : FNovaOrbitalObject()
	{
		Area  = A;
		Phase = P;
	}

	FNovaOrbitalObject(const TSharedPtr<struct FNovaSpacecraft>& S, float P) : FNovaOrbitalObject()
	{
		Spacecraft = S;
		Phase      = P;
	}

	FNovaOrbitalObject(const FNovaManeuver& M) : FNovaOrbitalObject()
	{
		Maneuver = MakeShared<FNovaManeuver>(M);
		Phase    = M.Phase;
	}

	FText GetText() const;

	// Object data
	const class UNovaArea*             Area;
	TSharedPtr<struct FNovaSpacecraft> Spacecraft;
	TSharedPtr<struct FNovaManeuver>   Maneuver;

	// Positioning
	float     Phase;
	FVector2D Position;
};

/** Batched data for drawing a spline */
struct FNovaBatchedSpline
{
	FVector2D    P0;
	FVector2D    P1;
	FVector2D    P2;
	FVector2D    P3;
	FLinearColor ColorInner;
	FLinearColor ColorOuter;
	float        WidthInner;
	float        WidthOuter;
};

/** Batched data for drawing a point */
struct FNovaBatchedPoint
{
	FVector2D          Pos;
	FLinearColor       Color;
	float              Radius;
	FNovaOrbitalObject Object;
};

/** Batched data for drawing a quad */
struct FNovaBatchedBrush
{
	FVector2D          Pos;
	const FSlateBrush* Brush;
	float              Scale;
};

/*----------------------------------------------------
    Orbital map
----------------------------------------------------*/

/** Orbital map drawer */
class SNovaOrbitalMap : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaOrbitalMap)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:
	SNovaOrbitalMap() : CurrentPreviewProgress(0)
	{}

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Inherited
	----------------------------------------------------*/

public:
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual int32 OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override;

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	/** Preview a spacecraft trajectory */
	void PreviewTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory, bool Immediate = false);

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Draw a planet */
	void AddPlanet(const FVector2D& Pos, const class UNovaPlanet* Planet);

	/** Draw a trajectory */
	void AddTrajectory(const FVector2D& Origin, const FNovaTrajectory& Trajectory, const struct FNovaSplineStyle& Style,
		bool IncludeCurrent = true, float Progress = 1.0f);

	/** Draw an orbit */
	TPair<FVector2D, FVector2D> AddOrbit(
		const FVector2D& Origin, const FNovaOrbit& Orbit, const TArray<FNovaOrbitalObject>& Objects, const struct FNovaSplineStyle& Style);

	/** Draw an orbit based on processed 2D parameters */
	TPair<FVector2D, FVector2D> AddOrbitInternal(
		const struct FNovaSplineOrbit& Orbit, TArray<FNovaOrbitalObject> Objects, const struct FNovaSplineStyle& Style);

	/** Draw an interactive orbital object on the map */
	void AddOrbitalObject(const FNovaOrbitalObject& Object, const FLinearColor& Color);

	/** Add test orbits */
	void AddTestOrbits();

	/** Interpolate a spline, returning the point at Alpha (0-1) over the spline defined by P0..P3 */
	static FVector2D DeCasteljauInterp(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float Alpha)
	{
		return DeCasteljauSplit(P0, P1, P2, P3, Alpha)[3];
	};

	/** Return the control points for the two splines created when cutting the spline defined by P0..P3 at Alpha (0-1) */
	static TArray<FVector2D> DeCasteljauSplit(
		const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float Alpha)
	{
		const float InvAlpha = 1.0f - Alpha;

		const FVector2D L = InvAlpha * P0 + Alpha * P1;
		const FVector2D M = InvAlpha * P1 + Alpha * P2;
		const FVector2D N = InvAlpha * P2 + Alpha * P3;

		const FVector2D P = InvAlpha * L + Alpha * M;
		const FVector2D Q = InvAlpha * M + Alpha * N;

		const FVector2D R = InvAlpha * P + Alpha * Q;

		return {P0, L, P, R, Q, N, P3};
	};

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	float                            TrajectoryPreviewDuration;

	// Local state
	TSharedPtr<FNovaTrajectory> CurrentPreviewTrajectory;
	float                       CurrentPreviewProgress;

	// Batching system
	TArray<FNovaBatchedSpline> BatchedSplines;
	TArray<FNovaBatchedPoint>  BatchedPoints;
	TArray<FNovaBatchedBrush>  BatchedBrushes;
};
