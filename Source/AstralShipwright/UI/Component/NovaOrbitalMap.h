// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "Actor/NovaActorTools.h"

#include "Game/NovaGameTypes.h"
#include "Game/NovaAsteroidSimulationComponent.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Widgets/SCompoundWidget.h"

/*----------------------------------------------------
    Internal structures
----------------------------------------------------*/

/** Batched data for drawing a spline */
struct FNovaBatchedSpline
{
	bool operator==(const FNovaBatchedSpline& Other) const
	{
		return P0 == Other.P0 && P1 == Other.P1 && P2 == Other.P2 && P3 == Other.P3 && ColorInner == Other.ColorInner &&
			   ColorOuter == Other.ColorOuter && WidthInner == Other.WidthInner && WidthOuter == Other.WidthOuter;
	}

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
	bool operator==(const FNovaBatchedPoint& Other) const
	{
		return Pos == Other.Pos && Color == Other.Color && Scale == Other.Scale;
	}

	FVector2D          Pos;
	FLinearColor       Color;
	float              Scale;
	const FSlateBrush* Brush;
};

/** Batched data for drawing a quad */
struct FNovaBatchedBrush
{
	bool operator==(const FNovaBatchedBrush& Other) const
	{
		return Pos == Other.Pos && Brush == Other.Brush;
	}

	FVector2D          Pos;
	const FSlateBrush* Brush;
};

/** Batched data for drawing a text */
struct FNovaBatchedText
{
	FVector2D              Pos;
	FText                  Text;
	const FTextBlockStyle* TextStyle;
};

/** Point of interest on the map */
struct FNovaOrbitalObject
{
	FNovaOrbitalObject() : Area(nullptr), SpacecraftIdentifier(FGuid()), Maneuver(nullptr)
	{}

	FNovaOrbitalObject(const class UNovaArea* A, const FVector2D& P) : FNovaOrbitalObject()
	{
		Area     = A;
		Position = P;
	}

	FNovaOrbitalObject(const FGuid& I, const FVector2D& P, bool IsAsteroid) : FNovaOrbitalObject()
	{
		if (IsAsteroid)
		{
			AsteroidIdentifier = I;
		}
		else
		{
			SpacecraftIdentifier = I;
		}
		Position = P;
	}

	FNovaOrbitalObject(const FNovaManeuver& M, const FVector2D& P) : FNovaOrbitalObject()
	{
		Maneuver = MakeShared<FNovaManeuver>(M);
		Position = P;
	}

	const FSlateBrush* GetBrush() const
	{
		if (Maneuver.IsValid())
		{
			return FNovaStyleSet::GetBrush("Map/SB_Maneuver");
		}
		else if (Area.IsValid())
		{
			return FNovaStyleSet::GetBrush("Map/SB_Area");
		}
		else if (AsteroidIdentifier.IsValid())
		{
			return FNovaStyleSet::GetBrush("Map/SB_Asteroid");
		}
		else
		{
			return FNovaStyleSet::GetBrush("Map/SB_Spacecraft");
		}
	}

	bool operator==(const FNovaOrbitalObject& Other) const
	{
		return Area == Other.Area && AsteroidIdentifier == Other.AsteroidIdentifier && SpacecraftIdentifier == Other.SpacecraftIdentifier &&
			   Maneuver == Other.Maneuver;
	}

	bool operator!=(const FNovaOrbitalObject& Other) const
	{
		return !operator==(Other);
	}

	// Object data
	TWeakObjectPtr<const class UNovaArea> Area;
	FGuid                                 AsteroidIdentifier;
	FGuid                                 SpacecraftIdentifier;
	TSharedPtr<FNovaManeuver>             Maneuver;

	// Position
	FVector2D Position;
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

	SLATE_BEGIN_ARGS(SNovaOrbitalMap) : _MenuManager(nullptr), _CurrentAlpha(1.0f)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)
	SLATE_ATTRIBUTE(float, CurrentAlpha);

	SLATE_END_ARGS()

public:
	SNovaOrbitalMap();

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/** Preview a spacecraft trajectory */
	void ShowTrajectory(const FNovaTrajectory& Trajectory, bool Immediate = false);

	/** Remove the trajectory preview */
	void ClearTrajectory()
	{
		CurrentPreviewTrajectory = FNovaTrajectory();
	}

	/** Get the preview trajectory */
	const FNovaTrajectory& GetPreviewTrajectory() const
	{
		return CurrentPreviewTrajectory;
	}

	/** Get hovered orbital objects */
	const TArray<FNovaOrbitalObject>& GetHoveredOrbitalObjects() const
	{
		return HoveredOrbitalObjects;
	}

	/** Pass horizontal input to the map */
	void HorizontalAnalogInput(float Value);

	/** Pass vertical input to the map */
	void VerticalAnalogInput(float Value);

	/** Reset the map */
	void Reset();

protected:
	/*----------------------------------------------------
	    High-level internals
	----------------------------------------------------*/

	/** Add areas to the map */
	void ProcessAreas(const FVector2D& Origin);

	/** Add asteroids to the map */
	void ProcessAsteroids(const FVector2D& Origin);

	/** Add player orbit to the map */
	void ProcessPlayerOrbit(const FVector2D& Origin);

	/** Add player trajectory to the map */
	void ProcessPlayerTrajectory(const FVector2D& Origin);

	/** Add the trajectory preview */
	void ProcessTrajectoryPreview(const FVector2D& Origin, float DeltaTime);

	/** Update the draw scale */
	void ProcessDrawScale(float DeltaTime);

protected:
	/*----------------------------------------------------
	    Map drawing implementation
	----------------------------------------------------*/

	/** Draw a planet */
	void AddPlanet(const FVector2D& Pos, const class UNovaCelestialBody* Planet);

	/** Draw a trajectory */
	void AddTrajectory(const FVector2D& Position, const struct FNovaTrajectory& Trajectory, const struct FNovaSpacecraft* Spacecraft,
		float Progress, const struct FNovaSplineStyle& OrbitStyle, const struct FNovaSplineStyle& ManeuverStyle);

	/** Draw an orbit */
	bool AddOrbit(const FVector2D& Position, const TSharedPtr<FVector2D>& StartPosition, const FNovaOrbitGeometry& Geometry,
		TArray<FNovaOrbitalObject>& Objects, const struct FNovaSplineStyle& Style, float InitialPhase = 0.0f);

	/** Draw an orbit based on processed 2D parameters */
	bool AddOrbitInternal(const struct FNovaSplineOrbit& Orbit, const struct FNovaSplineStyle& Style);

	/** Draw an interactive orbital object on the map */
	void AddOrbitalObject(FNovaOrbitalObject Object, const FLinearColor& Color);

	/** Get the base altitude */
	float GetObjectBaseAltitude(const UNovaCelestialBody* Body) const
	{
		return 0.25f * Body->Image.GetImageSize().X;
	}

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
	    Batch renderer
	----------------------------------------------------*/

	/** Remove all drawing elements */
	void ClearBatches();

	virtual int32 OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	TWeakObjectPtr<UNovaMenuManager> MenuManager;
	TAttribute<float>                CurrentAlpha;
	FNovaCameraInputFilter           CameraFilter;

	// Local settings
	float TrajectoryPreviewDuration;
	float TrajectoryZoomSpeed;
	float TrajectoryZoomAcceleration;
	float TrajectoryZoomSnappinness;
	float TrajectoryInflationRatio;

	// Local state
	FVector2D       CurrentOrigin;
	FVector2D       TargetPosition;
	FVector2D       CurrentPosition;
	FVector2D       CurrentVelocity;
	FNovaTrajectory CurrentPreviewTrajectory;
	float           CurrentPreviewProgress;
	float           CurrentDesiredSize;
	float           CurrentDrawScale;
	float           CurrentZoomSpeed;

	// Object system
	TArray<FNovaOrbitalObject> HoveredOrbitalObjects;

	// Batching system
	TArray<FNovaBatchedSpline> BatchedSplines;
	TArray<FNovaBatchedPoint>  BatchedPoints;
	TArray<FNovaBatchedBrush>  BatchedBrushes;
	TArray<FNovaBatchedText>   BatchedTexts;
};
