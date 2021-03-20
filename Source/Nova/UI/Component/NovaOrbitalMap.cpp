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
	else if (Spacecraft)
	{
		FString IDentifier = Spacecraft->Identifier.ToString(EGuidFormats::DigitsWithHyphens);
		int32   Index;
		if (IDentifier.FindLastChar('-', Index))
		{
			return FText::FromString(IDentifier.RightChop(Index + 1));
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
	ProcessSpacecraftOrbits(Origin);
	ProcessPlayerTrajectory(Origin);
	ProcessTrajectoryPreview(Origin, DeltaTime);
	ProcessDrawScale(DeltaTime);
}

void SNovaOrbitalMap::Set(const TSharedPtr<FNovaTrajectory>& Trajectory, bool Immediate)
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
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

	// Orbit merging structure
	struct FNovaMergedOrbitGeometry : FNovaOrbitGeometry
	{
		FNovaMergedOrbitGeometry(const FNovaOrbitGeometry& Base, const FNovaOrbitalObject& Object) : FNovaOrbitGeometry(Base)
		{
			Objects.Add(Object);
		}

		TArray<FNovaOrbitalObject> Objects;
	};
	TArray<FNovaMergedOrbitGeometry> MergedOrbitGeometries;

	// Merge orbits
	for (const auto AreaAndOrbitalLocation : OrbitalSimulation->GetAllAreasLocations())
	{
		const FNovaOrbitalLocation& OrbitalLocation = AreaAndOrbitalLocation.Value;
		const FNovaOrbitGeometry&   Geometry        = OrbitalLocation.Geometry;
		const float                 Phase           = OrbitalLocation.Phase;

		CurrentDesiredSize = FMath::Max(CurrentDesiredSize, Geometry.GetHighestAltitude());

		FNovaMergedOrbitGeometry* ExistingGeometry = MergedOrbitGeometries.FindByPredicate(
			[&](const FNovaMergedOrbitGeometry& OtherOrbit)
			{
				return OtherOrbit.StartAltitude == Geometry.StartAltitude && OtherOrbit.OppositeAltitude == Geometry.OppositeAltitude &&
					   OtherOrbit.StartPhase == Geometry.StartPhase && OtherOrbit.EndPhase == Geometry.EndPhase;
			});

		FNovaOrbitalObject Point = FNovaOrbitalObject(AreaAndOrbitalLocation.Key, Phase);

#if 0
		NLOG("%s at phase %f, orbit start phase %f", *Point.GetText(OrbitalSimulation->GetCurrentTime()).ToString(), Point.Phase,
			ExistingGeometry ? ExistingGeometry->StartPhase : Geometry.StartPhase);
#endif

		if (ExistingGeometry)
		{
			ExistingGeometry->Objects.Add(Point);
		}
		else
		{
			MergedOrbitGeometries.Add(FNovaMergedOrbitGeometry(Geometry, Point));
		}
	}

	// Draw merged orbits
	for (FNovaMergedOrbitGeometry& Geometry : MergedOrbitGeometries)
	{
		AddOrbit(Origin, Geometry, Geometry.Objects, FNovaSplineStyle(FLinearColor::White));
	}
}

void SNovaOrbitalMap::ProcessSpacecraftOrbits(const FVector2D& Origin)
{
	ANovaGameWorld*                  GameWorld         = ANovaGameWorld::Get(MenuManager.Get());
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

	// Add the current orbit
	for (const auto SpacecraftIdentifierAndOrbitalLocation : OrbitalSimulation->GetAllSpacecraftLocations())
	{
		const FGuid& Identifier = SpacecraftIdentifierAndOrbitalLocation.Key;

		if (SpacecraftIdentifierAndOrbitalLocation.Value.Geometry.IsValid() && !OrbitalSimulation->IsOnTrajectory(Identifier))
		{
			const FNovaOrbitalLocation& Location = SpacecraftIdentifierAndOrbitalLocation.Value;

			TArray<FNovaOrbitalObject> Objects;

			Objects.Add(FNovaOrbitalObject(GameWorld->GetSpacecraft(Identifier), Location.Phase));

			AddOrbit(Origin, Location.Geometry, Objects, FNovaSplineStyle(FLinearColor::Blue));
			CurrentDesiredSize = FMath::Max(CurrentDesiredSize, Location.Geometry.GetHighestAltitude());
		}
	}
}

void SNovaOrbitalMap::ProcessPlayerTrajectory(const FVector2D& Origin)
{
	ANovaGameWorld*                  GameWorld         = ANovaGameWorld::Get(MenuManager.Get());
	UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());

	// Add the current trajectory
	const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();
	if (PlayerTrajectory)
	{
		AddTrajectory(Origin, *PlayerTrajectory, FNovaSplineStyle(FLinearColor::Blue),
			GameWorld->GetSpacecraft(OrbitalSimulation->GetPlayerSpacecraftIdentifier()));
		CurrentDesiredSize = FMath::Max(CurrentDesiredSize, PlayerTrajectory->GetHighestAltitude());
	}
}

void SNovaOrbitalMap::ProcessTrajectoryPreview(const FVector2D& Origin, float DeltaTime)
{
	CurrentPreviewProgress += DeltaTime / TrajectoryPreviewDuration;
	CurrentPreviewProgress = FMath::Min(CurrentPreviewProgress, 1.0f);

	// Add the preview trajectory
	if (CurrentPreviewTrajectory.IsValid())
	{
		AddTrajectory(Origin, *CurrentPreviewTrajectory, FNovaSplineStyle(FLinearColor::Yellow), nullptr, CurrentPreviewProgress);
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

void SNovaOrbitalMap::AddTrajectory(const FVector2D& Position, const FNovaTrajectory& Trajectory, const FNovaSplineStyle& Style,
	const struct FNovaSpacecraft* Spacecraft, float Progress)
{
	const FNovaOrbitalLocation* SpacecraftLocation = nullptr;
	if (Spacecraft)
	{
		UNovaOrbitalSimulationComponent* OrbitalSimulation = UNovaOrbitalSimulationComponent::Get(MenuManager.Get());
		SpacecraftLocation                                 = OrbitalSimulation->GetSpacecraftLocation(Spacecraft->Identifier);
	}

	for (int32 TransferIndex = 0; TransferIndex < Trajectory.TransferOrbits.Num(); TransferIndex++)
	{
		const TArray<FNovaOrbitGeometry>& TransferOrbits = Trajectory.TransferOrbits;
		FNovaOrbitGeometry                Geometry       = TransferOrbits[TransferIndex];

		bool  SkipRemainingTransfers = false;
		float CurrentPhase = FMath::Lerp(TransferOrbits[0].StartPhase, TransferOrbits[TransferOrbits.Num() - 1].EndPhase, Progress);
		if (CurrentPhase < Geometry.EndPhase)
		{
			Geometry.EndPhase      = CurrentPhase;
			SkipRemainingTransfers = true;
		}

		TArray<FNovaOrbitalObject> Objects;
		for (const FNovaManeuver& Maneuver : Trajectory.Maneuvers)
		{
			Objects.Add(Maneuver);
		}

		if (SpacecraftLocation && Geometry.StartPhase <= SpacecraftLocation->Phase && SpacecraftLocation->Phase <= Geometry.EndPhase)
		{
#if 0
			NLOG("T%d : %s at phase %f on orbit with sphase %f, ephase %f", TransferIndex, *Spacecraft->Identifier.ToString(),
				SpacecraftLocation->Phase, Geometry.StartPhase, Geometry.EndPhase);
#endif

			Objects.Add(FNovaOrbitalObject(Spacecraft, SpacecraftLocation->Phase));
		}

		AddOrbit(Position, Geometry, Objects, Style);

		if (SkipRemainingTransfers)
		{
			break;
		}
	}
}

TPair<FVector2D, FVector2D> SNovaOrbitalMap::AddOrbit(
	const FVector2D& Position, const FNovaOrbitGeometry& Geometry, const TArray<FNovaOrbitalObject>& Objects, const FNovaSplineStyle& Style)
{
	const FVector2D LocalPosition = Position * CurrentDrawScale;

	const float  RadiusA       = CurrentDrawScale * Geometry.StartAltitude;
	const float  RadiusB       = CurrentDrawScale * Geometry.OppositeAltitude;
	const float  SemiMajorAxis = 0.5f * (RadiusA + RadiusB);
	const float  SemiMinorAxis = FMath::Sqrt(RadiusA * RadiusB);
	const float& Phase         = Geometry.StartPhase;
	const float& InitialAngle  = 0;
	const float  AngularLength = Geometry.EndPhase - Geometry.StartPhase;
	const float  Offset        = 0.5f * (RadiusB - RadiusA);

	return AddOrbitInternal(
		FNovaSplineOrbit(LocalPosition, SemiMajorAxis, SemiMinorAxis, Phase, InitialAngle, AngularLength, Offset), Objects, Style);
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
		ANovaGameWorld* GameWorld = ANovaGameWorld::Get(MenuManager.Get());

		DesiredObjectTexts.AddUnique(Object.GetText(GameWorld->GetCurrentTime()).ToString());
	}
}

TPair<FVector2D, FVector2D> SNovaOrbitalMap::AddOrbitInternal(
	const FNovaSplineOrbit& Orbit, TArray<FNovaOrbitalObject> Objects, const FNovaSplineStyle& Style)
{
	bool      FirstRenderedSpline = true;
	FVector2D InitialPosition     = FVector2D::ZeroVector;
	FVector2D FinalPosition       = FVector2D::ZeroVector;

	for (int32 SplineIndex = 0; SplineIndex < FMath::CeilToInt(Orbit.AngularLength / 90); SplineIndex++)
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
		if (CurrentStartAngle >= Orbit.AngularLength)
		{
			break;
		}
		if (CurrentEndAngle > Orbit.AngularLength)
		{
			float             Alpha         = FMath::Fmod(Orbit.AngularLength, 90.0f) / CurrentSegmentLength;
			TArray<FVector2D> ControlPoints = DeCasteljauSplit(P0, P1, P2, P3, Alpha);

			CurrentSegmentLength *= Alpha;

			P0 = ControlPoints[0];
			P1 = ControlPoints[1];
			P2 = ControlPoints[2];
			P3 = ControlPoints[3];
		}

		// Process points of interest
		for (FNovaOrbitalObject& Object : Objects)
		{
			const float CurrentSplineStartPhase = Orbit.Phase + CurrentStartAngle;
			const float CurrentSplineEndPhase   = Orbit.Phase + CurrentEndAngle;

			if (!Object.Positioned && Object.Phase >= CurrentSplineStartPhase && Object.Phase <= CurrentSplineEndPhase)
			{
				float Alpha = FMath::Fmod(Object.Phase - Orbit.Phase, 90.0f) / CurrentSegmentLength;
				if (Alpha == 0 && Object.Phase != CurrentSplineStartPhase)
				{
					Alpha = 1.0f;
				}

				Object.Position   = DeCasteljauInterp(P0, P1, P2, P3, Alpha);
				Object.Positioned = true;

#if 0
				NLOG("Positioning at phase %f -> %f (%f) -> %f, %fp // CSSP %f, CSEP %f, CurrentEndAngle %f", Object.Phase, Alpha,
					CurrentSegmentLength, Object.Position.X, Object.Position.Y, CurrentSplineStartPhase, CurrentSplineEndPhase,
					CurrentEndAngle);
#endif
			}
		}

		// Batch the spline segment
		if (SplineIndex < 4)
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
		if (Object.Positioned)
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
	NLOG("Painting %d brushes, %d splines, %d points, %d texts", BatchedBrushes.Num(), BatchedSplines.Num(), BatchedPoints.Num(),
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
