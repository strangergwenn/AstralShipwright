// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"

#include "Widgets/SCompoundWidget.h"


/*----------------------------------------------------
	Internal structures
----------------------------------------------------*/

/** Geometry of an orbit on the map */
struct FNovaSplineOrbit
{
	FNovaSplineOrbit(const FVector2D& Orig, float R)
		: Origin(Orig)
		, Width(R)
		, Height(R)
		, Phase(0)
		, InitialAngle(0)
		, AngularLength(360)
		, OriginOffset(0)
	{}

	FNovaSplineOrbit(const FVector2D& Orig, float W, float H, float P)
		: FNovaSplineOrbit(Orig, W)
	{
		Height = H;
		Phase = P;
	}

	FNovaSplineOrbit(const FVector2D& Orig, float W, float H, float P, float Initial, float Length, float Offset)
		: FNovaSplineOrbit(Orig, W, H, P)
	{
		InitialAngle = Initial;
		AngularLength = Length;
		OriginOffset = Offset;
	}

	FVector2D Origin;
	float Width;
	float Height;
	float Phase;
	float InitialAngle;
	float AngularLength;
	float OriginOffset;
};

/** Orbit drawing style */
struct FNovaSplineStyle
{
	FNovaSplineStyle()
		: ColorInner(FLinearColor::White)
		, ColorOuter(FLinearColor::Black)
		, WidthInner(1.0f)
		, WidthOuter(2.0f)
	{}

	FNovaSplineStyle(const FLinearColor& Color)
		: FNovaSplineStyle()
	{
		ColorInner = Color;
		ColorOuter = Color;
	}

	FLinearColor ColorInner;
	FLinearColor ColorOuter;
	float WidthInner;
	float WidthOuter;
};

/** Orbit drawing results */
struct FNovaSplineResults
{
	FVector2D InitialPosition;
	FVector2D FinalPosition;
	TArray<FVector2D> PointsOfInterest;
};

/** Batched data for drawing a point */
struct FNovaBatchedPoint
{
	FVector2D Pos;
	FLinearColor Color;
	float Radius;
};

/** Batched data for drawing a spline */
struct FNovaBatchedSpline
{
	FVector2D P0;
	FVector2D P1;
	FVector2D P2;
	FVector2D P3;
	FLinearColor ColorInner;
	FLinearColor ColorOuter;
	float WidthInner;
	float WidthOuter;
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

	void Construct(const FArguments& InArgs);


	/*----------------------------------------------------
		Inherited
	----------------------------------------------------*/

public:

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	virtual int32 OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;


	/*----------------------------------------------------
		Internals
	----------------------------------------------------*/

protected:

	/** Draw a full circular orbit around Origin of Radius */
	TArray<FVector2D> AddCircularOrbit(const FVector2D& Origin, float Radius,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style);

	/** Draw a partial circular orbit around Origin of Radius, starting at Phase over AngularLength */
	TArray<FVector2D> AddPartialCircularOrbit(const FVector2D& Origin, float Radius, float Phase, float InitialAngle, float AngularLength,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style);

	/** Draw a Hohmann transfer orbit around Origin from RadiusA to RadiusB, starting at Phase */
	TArray<FVector2D> AddTransferOrbit(const FVector2D& Origin, float RadiusA, float RadiusB, float Phase, float InitialAngle,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style);

	/** Draw an orbit or partial orbit */
	FNovaSplineResults AddOrbit(const FNovaSplineOrbit& Orbit, const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style);

	/** Draw a single point on the map */
	void AddPoint(const FVector2D& Pos, const FLinearColor& Color, float Radius = 4.0f);

	/** Interpolate a spline, returning the point at Alpha (0-1) over the spline defined by P0..P3 */
	static FVector2D DeCasteljauInterp(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float Alpha)
	{
		return DeCasteljauSplit(P0, P1, P2, P3, Alpha)[3];
	};

	/** Return the control points for the two splines created when cutting the spline defined by P0..P3 at Alpha (0-1) */
	static TArray<FVector2D> DeCasteljauSplit(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float Alpha)
	{
		const float InvAlpha = 1.0f - Alpha;

		const FVector2D L = InvAlpha * P0 + Alpha * P1;
		const FVector2D M = InvAlpha * P1 + Alpha * P2;
		const FVector2D N = InvAlpha * P2 + Alpha * P3;

		const FVector2D P = InvAlpha * L + Alpha * M;
		const FVector2D Q = InvAlpha * M + Alpha * N;

		const FVector2D R = InvAlpha * P + Alpha * Q;

		return { P0, L, P, R, Q, N , P3 };
	};


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Menu manager
	TWeakObjectPtr<UNovaMenuManager>              MenuManager;

	// Batching system
	TArray<FNovaBatchedPoint>                     BatchedPoints;
	TArray<FNovaBatchedSpline>                    BatchedSplines;

};
