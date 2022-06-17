// Astral Shipwright - Gwennaël Arbona

#include "NovaOrbitalMap.h"

#include "Game/NovaArea.h"
#include "Game/NovaGameState.h"
#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronMenuManager.h"

#include "Widgets/Layout/SBackgroundBlur.h"

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

	FNovaSplineOrbit(
		const FVector2D& Orig, const TSharedPtr<FVector2D>& SP, float W, float H, float P, float Initial, float Length, float Offset)
	{
		Origin        = Orig;
		StartPosition = SP;
		Width         = W;
		Height        = H;
		Phase         = P;
		InitialAngle  = Initial;
		AngularLength = Length;
		OriginOffset  = Offset;
	}

	FVector2D             Origin;
	TSharedPtr<FVector2D> StartPosition;
	float                 Width;
	float                 Height;
	float                 Phase;
	float                 InitialAngle;
	float                 AngularLength;
	float                 OriginOffset;
};

/** Orbit drawing style */
struct FNovaSplineStyle
{
	FNovaSplineStyle() : ColorInner(FLinearColor::White), ColorOuter(FLinearColor::Black), WidthInner(2.0f), WidthOuter(2.0f)
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
	// Settings
	MenuManager  = InArgs._MenuManager;
	CurrentAlpha = InArgs._CurrentAlpha;

	// Local settings
	TrajectoryPreviewDuration  = 2.0f;
	TrajectoryZoomSpeed        = 0.5f;
	TrajectoryZoomAcceleration = 1.0f;
	TrajectoryZoomSnappinness  = 10.0f;
	TrajectoryInflationRatio   = 1.2f;

	// Zoom
	MapScaleMin         = 0.5f;
	MapScaleMax         = 2.0f;
	MapScaleStep        = 0.3f;
	CurrentDesiredScale = MapScaleMax;

	// Camera filter settings, based on the defaults
	CameraFilter.Velocity *= 50;
	CameraFilter.Acceleration *= 15;
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaOrbitalMap::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Debug data
	FVector2D                       Origin        = FVector2D::ZeroVector;
	const class UNovaCelestialBody* DefaultPlanet = UNeutronAssetManager::Get()->GetDefaultAsset<UNovaCelestialBody>();

	// Reset state
	ClearBatches();
	CurrentDesiredSize = 100;
	HoveredOrbitalObjects.Empty();

	// Integrate analog input
	const float     PositionFreedom = 0.5f;
	const FVector2D HalfLocalSize   = GetTickSpaceGeometry().GetLocalSize() / 2;
	CameraFilter.ApplyFilter(CurrentPosition, CurrentVelocity, TargetPosition, DeltaTime, MenuManager->IsUsingGamepad());
	CurrentPosition.X = FMath::Clamp(CurrentPosition.X, PositionFreedom * -HalfLocalSize.X, PositionFreedom * HalfLocalSize.X);
	CurrentPosition.Y = FMath::Clamp(CurrentPosition.Y, PositionFreedom * -HalfLocalSize.Y, PositionFreedom * HalfLocalSize.Y);
	CurrentOrigin     = HalfLocalSize + CurrentPosition;

	// Step the preview simulation
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());
	const FNovaTime                  CurrentTrajectoryDuration =
        CurrentPreviewTrajectory.IsValid() ? CurrentPreviewTrajectory.TotalTravelDuration : FNovaTime();
	OrbitalSimulation->UpdateSimulationForPreview(CurrentPreviewProgress * CurrentTrajectoryDuration);

	// Run orbital map processes
	AddPlanet(Origin, DefaultPlanet);
	ProcessAreas(Origin);
	ProcessAsteroids(Origin);
	ProcessSpacecraftLocations(Origin);
	ProcessPlayerTrajectory(Origin);
	ProcessTrajectoryPreview(Origin, DeltaTime);

	// Run display processes
	ProcessDrawScale(DeltaTime);
	if (MenuManager->IsUsingGamepad())
	{
		FNovaBatchedPoint Point;
		Point.Pos   = -CurrentPosition;
		Point.Color = FLinearColor::White;
		Point.Scale = 1.0f;
		Point.Brush = FNeutronStyleSet::GetBrush("Map/SB_Crosshair");
		BatchedPoints.AddUnique(Point);
	}
}

void SNovaOrbitalMap::ShowTrajectory(const FNovaTrajectory& Trajectory, const TArray<FNovaOrbitalObject>& ObjectsToShow, bool Immediate)
{
	if (Trajectory.IsValid())
	{
		CurrentPreviewTrajectory = Trajectory;
		CurrentPreviewProgress   = Immediate ? TrajectoryPreviewDuration : 0;
		CurrentPreviewObjects    = ObjectsToShow;
	}
}

void SNovaOrbitalMap::HorizontalAnalogInput(float Value)
{
	if (MenuManager->IsUsingGamepad())
	{
		TargetPosition.X -= Value;
	}
	else
	{
		TargetPosition.X += Value;
	}
}

void SNovaOrbitalMap::VerticalAnalogInput(float Value)
{
	if (MenuManager->IsUsingGamepad())
	{
		TargetPosition.Y += Value;
	}
	else
	{
		TargetPosition.Y -= Value;
	}
}

void SNovaOrbitalMap::ZoomIn()
{
	CurrentDesiredScale = FMath::Max(CurrentDesiredScale - MapScaleStep, MapScaleMin);

	NLOG("SNovaOrbitalMap::ZoomIn : scale %.02f, pos %.02f %.02f", CurrentDesiredScale, CurrentPosition.X, CurrentPosition.Y);
}

void SNovaOrbitalMap::ZoomOut()
{
	CurrentDesiredScale = FMath::Min(CurrentDesiredScale + MapScaleStep, MapScaleMax);

	const FVector2D CursorPosition = GetTickSpaceGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());

	NLOG("SNovaOrbitalMap::ZoomOut : scale %.02f, pos %.02f %.02f", CurrentDesiredScale, CurrentPosition.X, CurrentPosition.Y);
}

void SNovaOrbitalMap::Reset()
{
	CurrentPosition     = FVector2D::ZeroVector;
	CurrentDesiredScale = MapScaleMax;

	const FVector2D CursorPosition = GetTickSpaceGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());

	NLOG("SNovaOrbitalMap::Reset : scale %.02f, pos %.02f %.02f", CurrentDesiredScale, CurrentPosition.X, CurrentPosition.Y);
}

/*----------------------------------------------------
    High-level internals
----------------------------------------------------*/

void SNovaOrbitalMap::ProcessAreas(const FVector2D& Origin)
{
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());
	if (IsValid(OrbitalSimulation))
	{
		FNovaSplineStyle AsteroidStyle(FLinearColor(1, 1, 1, 0.5f));

		for (const auto AreaAndOrbitalLocation : OrbitalSimulation->GetPreviewAreasLocations())
		{
			const FNovaOrbitalLocation& OrbitalLocation = AreaAndOrbitalLocation.Value;
			const FNovaOrbitGeometry&   Geometry        = OrbitalLocation.Geometry;

			UpdateDesiredSize(Geometry.GetHighestAltitude());

			float              BaseAltitude = GetObjectBaseAltitude(Geometry.Body);
			FNovaOrbitalObject AreaObject =
				FNovaOrbitalObject(AreaAndOrbitalLocation.Key, OrbitalLocation.GetCartesianLocation(BaseAltitude));

			if (ShouldDisplayObject(AreaObject))
			{
				if (CurrentPreviewTrajectory.IsValid())
				{
					TArray<FNovaOrbitalObject> Objects;
					Objects.Add(AreaObject);
					AddOrbit(Origin, nullptr, OrbitalLocation.Geometry, Objects, AsteroidStyle);
				}
				else
				{
					AddOrbitalObject(AreaObject, AsteroidStyle.ColorOuter);
				}
			}
		}
	}
}

void SNovaOrbitalMap::ProcessAsteroids(const FVector2D& Origin)
{
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());
	if (IsValid(OrbitalSimulation))
	{
		FNovaSplineStyle AreaStyle(FLinearColor(1, 1, 1, 0.5f));

		for (const auto AsteroidAndOrbitalLocation : OrbitalSimulation->GetPreviewAsteroidsLocations())
		{
			const FNovaOrbitalLocation& OrbitalLocation = AsteroidAndOrbitalLocation.Value;
			const FNovaOrbitGeometry&   Geometry        = OrbitalLocation.Geometry;

			UpdateDesiredSize(Geometry.GetHighestAltitude());

			float              BaseAltitude = GetObjectBaseAltitude(Geometry.Body);
			FNovaOrbitalObject AsteroidObject =
				FNovaOrbitalObject(AsteroidAndOrbitalLocation.Key, OrbitalLocation.GetCartesianLocation(BaseAltitude), true);

			if (ShouldDisplayObject(AsteroidObject))
			{
				if (CurrentPreviewTrajectory.IsValid())
				{
					TArray<FNovaOrbitalObject> Objects;
					Objects.Add(AsteroidObject);
					AddOrbit(Origin, nullptr, OrbitalLocation.Geometry, Objects, AreaStyle);
				}
				else
				{
					AddOrbitalObject(AsteroidObject, AreaStyle.ColorOuter);
				}
			}
		}
	}
}

void SNovaOrbitalMap::ProcessSpacecraftLocations(const FVector2D& Origin)
{
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());
	if (!IsValid(OrbitalSimulation))
	{
		return;
	}

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FNovaSplineStyle         OrbitStyle(MenuManager->GetHighlightColor());
	OrbitStyle.ColorInner = FLinearColor::Black;
	OrbitStyle.WidthOuter = 5;
	OrbitStyle.WidthInner = 1;

	// Add the current orbit
	for (const auto SpacecraftIdentifierAndOrbitalLocation : OrbitalSimulation->GetPreviewSpacecraftLocations())
	{
		const FGuid& Identifier = SpacecraftIdentifierAndOrbitalLocation.Key;

		if (SpacecraftIdentifierAndOrbitalLocation.Value.Geometry.IsValid())
		{
			const FNovaOrbitalLocation& Location = SpacecraftIdentifierAndOrbitalLocation.Value;

			float              BaseAltitude = GetObjectBaseAltitude(Location.Geometry.Body);
			FNovaOrbitalObject Object       = FNovaOrbitalObject(Identifier, Location.GetCartesianLocation(BaseAltitude), false);

			// Player orbit
			const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
			if (GameState->GetPlayerSpacecraftIdentifiers().Contains(Identifier))
			{
				if (!OrbitalSimulation->IsOnStartedTrajectory(Identifier))
				{
					TArray<FNovaOrbitalObject> Objects;
					if (!CurrentPreviewTrajectory.IsValid())
					{
						Objects.Add(Object);
					}

					AddOrbit(Origin, nullptr, Location.Geometry, Objects, OrbitStyle);
				}
			}

			// Spacecraft position
			else if (ShouldDisplayObject(Object))
			{
				AddOrbitalObject(Object, FLinearColor::White);
			}

			UpdateDesiredSize(Location.Geometry.GetHighestAltitude());
		}
	}
}

void SNovaOrbitalMap::ProcessPlayerTrajectory(const FVector2D& Origin)
{
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());
	if (!IsValid(OrbitalSimulation))
	{
		return;
	}

	// Style
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FNovaSplineStyle         OrbitStyle(MenuManager->GetHighlightColor());
	FNovaSplineStyle         ManeuverStyle = OrbitStyle;
	ManeuverStyle.WidthInner               = 4.0f;
	ManeuverStyle.WidthOuter               = 4.0f;

	// Add the current trajectory
	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();
	if (PlayerTrajectory)
	{
		const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
		AddTrajectory(Origin, *PlayerTrajectory, GameState->GetSpacecraft(GameState->GetPlayerSpacecraftIdentifier()), 1.0f, OrbitStyle,
			ManeuverStyle);

		UpdateDesiredSize(PlayerTrajectory->GetHighestAltitude());
	}
}

void SNovaOrbitalMap::ProcessTrajectoryPreview(const FVector2D& Origin, float DeltaTime)
{
	// Style
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FNovaSplineStyle         OrbitStyle(Theme.ContrastingColor);
	FNovaSplineStyle         ManeuverStyle(Theme.ContrastingColor);
	ManeuverStyle.WidthInner = 4.0f;
	ManeuverStyle.WidthOuter = 4.0f;

	// Add the preview trajectory
	if (CurrentPreviewTrajectory.IsValid())
	{
		CurrentPreviewProgress += DeltaTime / TrajectoryPreviewDuration;
		CurrentPreviewProgress = FMath::Min(CurrentPreviewProgress, 1.0f);

		AddTrajectory(Origin, CurrentPreviewTrajectory, nullptr, CurrentPreviewProgress, OrbitStyle, ManeuverStyle);

		UpdateDesiredSize(CurrentPreviewTrajectory.GetHighestAltitude());
	}
	else
	{
		CurrentPreviewProgress = 0;
	}
}

void SNovaOrbitalMap::ProcessDrawScale(float DeltaTime)
{
	CurrentDesiredSize *= CurrentDesiredScale * TrajectoryInflationRatio;

	const float TargetScale =
		FMath::Min(GetTickSpaceGeometry().GetLocalSize().X, GetTickSpaceGeometry().GetLocalSize().Y) / CurrentDesiredSize;
	float ScaleDelta        = (TargetScale - CurrentDrawScale) * TrajectoryZoomSnappinness;
	float ScaleAcceleration = CurrentZoomSpeed - ScaleDelta;
	ScaleAcceleration       = FMath::Clamp(ScaleAcceleration, -TrajectoryZoomAcceleration, TrajectoryZoomAcceleration);

	CurrentZoomSpeed += ScaleAcceleration * DeltaTime;
	CurrentZoomSpeed = FMath::Clamp(CurrentZoomSpeed, -TrajectoryZoomSpeed, TrajectoryZoomSpeed);
	CurrentDrawScale += ScaleDelta * DeltaTime;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void SNovaOrbitalMap::AddPlanet(const FVector2D& Pos, const class UNovaCelestialBody* Planet)
{
	NCHECK(Planet);

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	// Planet brush
	FNovaBatchedBrush Brush;
	Brush.Brush = &Planet->Image;
	Brush.Pos   = Pos * CurrentDrawScale;
	BatchedBrushes.AddUnique(Brush);
}

void SNovaOrbitalMap::AddTrajectory(const FVector2D& Position, const FNovaTrajectory& Trajectory, const FNovaSpacecraft* Spacecraft,
	float Progress, const FNovaSplineStyle& OrbitStyle, const FNovaSplineStyle& ManeuverStyle)
{
	NCHECK(Trajectory.IsValid());

	const ANovaGameState*            GameState         = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

	// Prepare our work
	TSharedPtr<FVector2D>     AbsolutePosition;
	float                     CurrentSpacecraftPhase = 0;
	const TArray<FNovaOrbit>& Transfers              = Trajectory.Transfers;
	float CurrentProgressPhase = FMath::Lerp(Transfers[0].Geometry.StartPhase, Transfers[Transfers.Num() - 1].Geometry.EndPhase, Progress);

	// Position the player spacecraft
	TArray<FNovaOrbitalObject> Objects;
	if (Spacecraft)
	{
		const FNovaOrbitalLocation* SpacecraftLocation = OrbitalSimulation->GetSpacecraftLocation(Spacecraft->Identifier);

		if (SpacecraftLocation)
		{
#if 0
			NLOG("SNovaOrbitalMap::AddTrajectory : %s at phase %f ", *Spacecraft->Identifier.ToString(), SpacecraftLocation->Phase);
#endif

			// Determine the absolute position
			float BaseAltitude = GetObjectBaseAltitude(SpacecraftLocation->Geometry.Body);
			AbsolutePosition   = MakeShared<FVector2D>(SpacecraftLocation->GetCartesianLocation(BaseAltitude));
			Objects.Add(FNovaOrbitalObject(Spacecraft->Identifier, *AbsolutePosition, false));

			// Determine the physical angle
			const FVector2D RelativePosition = SpacecraftLocation->GetCartesianLocation<false>(BaseAltitude);
			float           CartesianAngle =
				-FMath::RadiansToDegrees(FVector(RelativePosition.X, RelativePosition.Y, 0).GetSafeNormal().HeadingAngle());

			// Match with the previous value
			float       MinimumAngleDistance = FLT_MAX;
			const float MaximumAngleSteps    = 2 * FMath::CeilToInt(SpacecraftLocation->Phase / 360);
			for (int32 i = -MaximumAngleSteps; i < MaximumAngleSteps; i++)
			{
				float NewAngle    = CartesianAngle + i * 360.0f;
				float NewDistance = FMath::Abs(NewAngle - SpacecraftLocation->Phase);

				if (NewDistance < MinimumAngleDistance)
				{
					CurrentSpacecraftPhase = NewAngle;
					MinimumAngleDistance   = NewDistance;
				}
				else
				{
					break;
				}
			}

#if 0
			NLOG("SNovaOrbitalMap::AddTrajectory : %f / [%f,%f] -> %f -> %f", SpacecraftLocation->Phase, RelativePosition.X,
				RelativePosition.Y, CartesianAngle, CurrentSpacecraftPhase);
#endif
		}
	}

	// Draw orbit segments
	for (const FNovaOrbit& Transfer : Trajectory.Transfers)
	{
		FNovaOrbitGeometry Geometry = Transfer.Geometry;

		// Detect the starting point
		TSharedPtr<FVector2D> StartPosition;
		if (GameState->GetCurrentTime() > Transfer.InsertionTime)
		{
			StartPosition = AbsolutePosition;
		}

		// Detect the final transfer
		bool SkipRemainingTransfers = false;
		if (CurrentProgressPhase < Geometry.EndPhase)
		{
			Geometry.EndPhase      = CurrentProgressPhase;
			SkipRemainingTransfers = true;
		}

		if (AddOrbit(Position, StartPosition, Geometry, Objects, OrbitStyle, CurrentSpacecraftPhase) && StartPosition)
		{
			AbsolutePosition = nullptr;
		}

		if (SkipRemainingTransfers)
		{
			break;
		}
	}

	// Get maneuvers
	TArray<FNovaOrbitalObject> ManeuverObjects;
	for (const FNovaManeuver& Maneuver : Trajectory.Maneuvers)
	{
		const FNovaOrbitalLocation ManeuverLocation = Trajectory.GetLocation(Maneuver.Time);

		if ((Maneuver.Time + Maneuver.Duration > GameState->GetCurrentTime()) && ManeuverLocation.Phase < CurrentProgressPhase)
		{
			double BaseAltitude = GetObjectBaseAltitude(Trajectory.Transfers[0].Geometry.Body);
			ManeuverObjects.Add(FNovaOrbitalObject(Maneuver, ManeuverLocation.GetCartesianLocation(BaseAltitude)));
		}
	}

	// Draw maneuver arcs
	for (const FNovaManeuver& Maneuver : Trajectory.Maneuvers)
	{
		for (const FNovaOrbit& Transfer : Trajectory.GetRelevantOrbitsForManeuver(Maneuver))
		{
			FNovaOrbitGeometry ManeuverGeometry = Transfer.Geometry;
			float              ManeuverEndPhase = Transfer.GetPhase<false>(Maneuver.Time + Maneuver.Duration);
			ManeuverGeometry.EndPhase           = FMath::Min(ManeuverGeometry.EndPhase, ManeuverEndPhase);
			ManeuverGeometry.EndPhase           = FMath::Min(ManeuverGeometry.EndPhase, CurrentProgressPhase);

			if (CurrentProgressPhase > Maneuver.Phase && Maneuver.Phase < ManeuverGeometry.EndPhase)
			{
				AddOrbit(Position, nullptr, ManeuverGeometry, ManeuverObjects, ManeuverStyle,
					FMath::Max(Maneuver.Phase, CurrentSpacecraftPhase));
			}
		}
	}
}

bool SNovaOrbitalMap::AddOrbit(const FVector2D& Position, const TSharedPtr<FVector2D>& StartPosition, const FNovaOrbitGeometry& Geometry,
	TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style, float InitialPhase)
{
	const FVector2D LocalPosition = Position * CurrentDrawScale;

	const float RadiusA       = CurrentDrawScale * (GetObjectBaseAltitude(Geometry.Body) + Geometry.StartAltitude);
	const float RadiusB       = CurrentDrawScale * (GetObjectBaseAltitude(Geometry.Body) + Geometry.OppositeAltitude);
	const float SemiMajorAxis = 0.5f * (RadiusA + RadiusB);
	const float SemiMinorAxis = FMath::Sqrt(RadiusA * RadiusB);
	const float Phase         = Geometry.StartPhase;
	const float InitialAngle  = InitialPhase - Geometry.StartPhase;
	const float AngularLength = Geometry.EndPhase - Geometry.StartPhase;
	const float Offset        = 0.5f * (RadiusB - RadiusA);

	for (const FNovaOrbitalObject& Object : Objects)
	{
		AddOrbitalObject(Object, Style.ColorOuter);
	}

	return AddOrbitInternal(
		FNovaSplineOrbit(LocalPosition, StartPosition, SemiMajorAxis, SemiMinorAxis, Phase, InitialAngle, AngularLength, Offset), Style);
}

void SNovaOrbitalMap::AddOrbitalObject(FNovaOrbitalObject Object, const FLinearColor& Color)
{
	Object.Position *= CurrentDrawScale;

	// Check for hover
	float HoverRadius = 50;
	bool  IsObjectHovered;
	if (MenuManager->IsUsingGamepad())
	{
		IsObjectHovered = (CurrentOrigin + Object.Position - GetTickSpaceGeometry().GetLocalSize() / 2).Size() < HoverRadius;
	}
	else
	{
		IsObjectHovered =
			(CurrentOrigin + Object.Position - GetTickSpaceGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos())).Size() <
			HoverRadius;
	}

	// Add the point
	FNovaBatchedPoint Point;
	Point.Pos   = Object.Position;
	Point.Color = Color;
	Point.Scale = IsObjectHovered ? 2.0f : 1.5f;
	Point.Brush = Object.GetBrush();
	BatchedPoints.AddUnique(Point);

	// Add the text
	if (Object.Area.IsValid())
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		FNovaBatchedText Text;
		Text.Text      = Object.Area->Name.ToUpper();
		Text.Pos       = Point.Pos - FVector2D(0, 32);
		Text.TextStyle = &Theme.MainFont;
		BatchedTexts.Add(Text);
	}

	if (IsObjectHovered)
	{
		HoveredOrbitalObjects.AddUnique(Object);
	}
}

bool SNovaOrbitalMap::AddOrbitInternal(const FNovaSplineOrbit& Orbit, const FNovaSplineStyle& Style)
{
	int32 RenderedSplineCount = 0;

	// Ignore orbits that are empty or completely erased by the initial angle
	if (Orbit.AngularLength <= 0 || Orbit.InitialAngle > Orbit.AngularLength)
	{
		return false;
	}

	// Strip orbits down to the minimum length to improve performance (Orbit.AngularLength can be +inf)
	const float AngularLength = FMath::IsFinite(Orbit.AngularLength) ? Orbit.AngularLength : 360;

	for (int32 SplineIndex = 0; SplineIndex < FMath::CeilToInt(AngularLength / 90.0f); SplineIndex++)
	{
		// Useful constants
		const float      CurrentStartAngle = SplineIndex * 90.0f;
		const float      CurrentEndAngle   = CurrentStartAngle + 90.0f;
		constexpr double CircleRatio       = 0.552284749831;

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
		float RelativeInitialAngle = 0;
		if (Orbit.InitialAngle >= CurrentEndAngle)
		{
			continue;
		}
		else if (Orbit.InitialAngle > CurrentStartAngle)
		{
			RelativeInitialAngle            = FMath::Fmod(Orbit.InitialAngle, 90.0f);
			TArray<FVector2D> ControlPoints = DeCasteljauSplit(P0, P1, P2, P3, RelativeInitialAngle / 90.0f);
			CurrentSegmentLength            = 90.0f - RelativeInitialAngle;

			P0 = ControlPoints[3];
			P1 = ControlPoints[4];
			P2 = ControlPoints[5];
			P3 = ControlPoints[6];
		}

		// Split the curve to account for angular length
		if (CurrentStartAngle >= AngularLength)
		{
			break;
		}
		if (CurrentEndAngle > AngularLength)
		{
			float             Alpha         = FMath::Fmod(AngularLength - RelativeInitialAngle, 90.0f) / CurrentSegmentLength;
			TArray<FVector2D> ControlPoints = DeCasteljauSplit(P0, P1, P2, P3, Alpha);

			CurrentSegmentLength *= Alpha;

			P0 = ControlPoints[0];
			P1 = ControlPoints[1];
			P2 = ControlPoints[2];
			P3 = ControlPoints[3];
		}

		if (RenderedSplineCount == 0 && Orbit.StartPosition)
		{
			P0 = CurrentDrawScale * (*Orbit.StartPosition);
		}

		// Batch the spline segment if we haven't done a full circle yet, including partial start & end segments
		if (RenderedSplineCount < 6)
		{
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
		}

		RenderedSplineCount++;
	}

	return (RenderedSplineCount > 0);
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
	// Early abort when no position is available
	if (CurrentOrigin.Size() == 0)
	{
		return SCompoundWidget::OnPaint(
			PaintArgs, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

#if 0
	NLOG("Painting %d brushes, %d splines, %d points, %d texts / pos %f, %f scale %f", BatchedBrushes.Num(), BatchedSplines.Num(),
		BatchedPoints.Num(), BatchedTexts.Num(), CurrentOrigin.X, CurrentOrigin.Y, CurrentDrawScale);
#endif

	// Draw batched brushes
	for (const FNovaBatchedBrush& Brush : BatchedBrushes)
	{
		NCHECK(Brush.Brush);
		FVector2D    BrushSize = Brush.Brush->GetImageSize() * CurrentDrawScale;
		FLinearColor BrushColor(1.0f, 1.0f, 1.0f, CurrentAlpha.Get());

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(BrushSize, FSlateLayoutTransform(CurrentOrigin + Brush.Pos - BrushSize / 2)), Brush.Brush,
			ESlateDrawEffect::NoPixelSnapping, BrushColor);
	}

	// Draw batched splines
	for (const FNovaBatchedSpline& Spline : BatchedSplines)
	{
		FLinearColor ColorOuter = Spline.ColorOuter;
		ColorOuter.A            = CurrentAlpha.Get();
		FLinearColor ColorInner = Spline.ColorInner;
		ColorInner.A            = CurrentAlpha.Get();

		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
			CurrentOrigin + Spline.P0, CurrentOrigin + Spline.P1, CurrentOrigin + Spline.P2, CurrentOrigin + Spline.P3,
			Spline.WidthOuter * AllottedGeometry.Scale, ESlateDrawEffect::NoPixelSnapping, ColorOuter);

		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
			CurrentOrigin + Spline.P0, CurrentOrigin + Spline.P1, CurrentOrigin + Spline.P2, CurrentOrigin + Spline.P3,
			Spline.WidthInner * AllottedGeometry.Scale, ESlateDrawEffect::NoPixelSnapping, ColorInner);
	}

	// Draw batched points
	for (const FNovaBatchedPoint& Point : BatchedPoints)
	{
		FVector2D BrushSize = Point.Brush->GetImageSize() * Point.Scale;

		FLinearColor Color = Point.Color;
		Color.A            = CurrentAlpha.Get();

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(BrushSize, FSlateLayoutTransform(CurrentOrigin + Point.Pos - BrushSize / 2)), Point.Brush,
			ESlateDrawEffect::NoPixelSnapping, Color);
	}

	// Draw batched texts
	for (const FNovaBatchedText& Text : BatchedTexts)
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		FVector2D                           TextSize           = FontMeasureService->Measure(Text.Text, Text.TextStyle->Font);
		FLinearColor                        TextColor(1.0f, 1.0f, 1.0f, CurrentAlpha.Get());

		FPaintGeometry TextGeometry =
			AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(CurrentOrigin + Text.Pos - TextSize / 2));

		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId + 4, TextGeometry, Text.Text, Text.TextStyle->Font, ESlateDrawEffect::NoPixelSnapping, TextColor);
	}

	return SCompoundWidget::OnPaint(PaintArgs, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

#undef LOCTEXT_NAMESPACE
