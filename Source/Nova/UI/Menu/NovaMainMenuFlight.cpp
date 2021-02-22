// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameInstance.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/UI/Component/NovaLargeButton.h"

#include "Nova/Nova.h"

#include "Slate/SRetainerWidget.h"


#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"


/*----------------------------------------------------
	Orbital map // DEBUG // TODO REMOVE
----------------------------------------------------*/

class SNovaOrbitalMap : public SCompoundWidget
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaOrbitalMap)
	{}

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{}

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	/** Internal Slate arguments for drawing */
	struct FNovaSlateArguments
	{
		FNovaSlateArguments(const FGeometry& Geometry, FSlateWindowElementList& List, int32 Id)
			: AllottedGeometry(Geometry)
			, OutDrawElements(List)
			, LayerId(Id)
		{}

		const FGeometry& AllottedGeometry;
		FSlateWindowElementList& OutDrawElements;
		int32 LayerId;
	};

	/** Geometry of an orbit on the map */
	struct FNovaOrbitGeometry
	{
		FNovaOrbitGeometry(const FVector2D& Orig, float R)
			: Origin(Orig)
			, Width(R)
			, Height(R)
			, Phase(0)
			, InitialAngle(0)
			, AngularLength(360)
			, OriginOffset(0)
		{}

		FNovaOrbitGeometry(const FVector2D& Orig, float W, float H, float P)
			: FNovaOrbitGeometry(Orig, W)
		{
			Height = H;
			Phase = P;
		}

		FNovaOrbitGeometry(const FVector2D& Orig, float W, float H, float P, float Initial, float Length, float Offset)
			: FNovaOrbitGeometry(Orig, W, H, P)
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
			, WidthFull(2.0f)
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
		float WidthFull;
	};

	/** Orbit drawing results */
	struct FNovaSplineResults
	{
		FVector2D InitialPosition;
		FVector2D FinalPosition;
		TArray<FVector2D> PointsOfInterest;
	};

	/** Draw a single point on the map */
	void DrawPoint(const FNovaSlateArguments& SlateArgs, const FVector2D& Pos, const FLinearColor& Color, float Radius = 4.0f) const
	{
		FSlateColorBrush WhiteBox = FSlateColorBrush(FLinearColor::White);
		const FVector2D BezierPointRadius = FVector2D(Radius, Radius);

		FSlateDrawElement::MakeBox(
			SlateArgs.OutDrawElements,
			SlateArgs.LayerId,
			SlateArgs.AllottedGeometry.ToPaintGeometry(2 * BezierPointRadius, FSlateLayoutTransform(Pos - BezierPointRadius)),
			&WhiteBox,
			ESlateDrawEffect::None,
			Color
		);
	};

	/** Interpolate a spline, returning the point at Alpha (0-1) over the spline defined by P0..P3 */
	FVector2D DeCasteljauInterp(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float Alpha) const
	{
		return DeCasteljauSplit(P0, P1, P2, P3, Alpha)[3];
	};

	/** Return the control points for the two splines created when cutting the spline defined by P0..P3 at Alpha (0-1) */
	TArray<FVector2D> DeCasteljauSplit(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, float Alpha) const
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

	/** Draw an orbit or partial orbit */
	FNovaSplineResults DrawOrbit(const FNovaSlateArguments& SlateArgs, const FNovaOrbitGeometry& Orbit, const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		bool FirstRenderedSpline = true;
		FNovaSplineResults Results;

		for (int32 SplineIndex = 0; SplineIndex < 4; SplineIndex++)
		{
			// Useful constants
			const float CurrentStartAngle = SplineIndex * 90.0f;
			const float CurrentEndAngle = CurrentStartAngle + 90.0f;
			constexpr float CircleRatio = 0.55228475f;

			// Define the control points for a 90° segment of the orbit
			FVector2D BezierPoints[4];
			const float FirstAxis = SplineIndex % 2 == 0 ? Orbit.Width : Orbit.Height;
			const float SecondAxis = SplineIndex % 2 == 0 ? Orbit.Height : Orbit.Width;
			BezierPoints[0] = FVector2D(FirstAxis, 0.5f);
			BezierPoints[3] = FVector2D(-0.5f, -SecondAxis);
			BezierPoints[1] = BezierPoints[0] - FVector2D(0, CircleRatio * SecondAxis);
			BezierPoints[2] = BezierPoints[3] + FVector2D(CircleRatio * FirstAxis, 0);

			// Rotate all segments around the orbit
			auto Transform = [&Orbit, SplineIndex](const FVector2D& ControlPoint)
			{
				return Orbit.Origin
					+ FVector2D(-Orbit.OriginOffset, 0).GetRotated(-Orbit.Phase)
					+ ControlPoint.GetRotated(-Orbit.Phase - SplineIndex * 90.0f);
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
				const float RelativeInitialAngle = FMath::Fmod(Orbit.InitialAngle, 90.0f);
				TArray<FVector2D> ControlPoints = DeCasteljauSplit(P0, P1, P2, P3, RelativeInitialAngle / 90.0f);
				P0 = ControlPoints[3];
				P1 = ControlPoints[4];
				P2 = ControlPoints[5];
				P3 = ControlPoints[6];
				CurrentSegmentLength = 90.0f - RelativeInitialAngle;
			}

			// Split the curve to account for angular length
			if (Orbit.AngularLength <= CurrentStartAngle)
			{
				break;
			}
			if (Orbit.AngularLength < CurrentEndAngle)
			{
				float Alpha = FMath::Fmod(Orbit.AngularLength, 90.0f) / CurrentSegmentLength;
				TArray<FVector2D> ControlPoints = DeCasteljauSplit(P0, P1, P2, P3, Alpha);
				P0 = ControlPoints[0];
				P1 = ControlPoints[1];
				P2 = ControlPoints[2];
				P3 = ControlPoints[3];
			}

			// Draw the spline segment with a shadow
			FSlateDrawElement::MakeCubicBezierSpline(
				SlateArgs.OutDrawElements,
				SlateArgs.LayerId,
				SlateArgs.AllottedGeometry.ToPaintGeometry(),
				P0, P1, P2, P3,
				Style.WidthFull * SlateArgs.AllottedGeometry.Scale,
				ESlateDrawEffect::None,
				Style.ColorOuter
			);
			FSlateDrawElement::MakeCubicBezierSpline(
				SlateArgs.OutDrawElements,
				SlateArgs.LayerId,
				SlateArgs.AllottedGeometry.ToPaintGeometry(),
				P0, P1, P2, P3,
				Style.WidthInner * SlateArgs.AllottedGeometry.Scale,
				ESlateDrawEffect::None,
				Style.ColorInner
			);

			// Report the initial and final positions
			if (FirstRenderedSpline)
			{
				Results.InitialPosition = P0;
				FirstRenderedSpline = false;
			}
			Results.FinalPosition = P3;
		}

		return Results;
	}

	/** Draw a full circular orbit around Origin of Radius */
	void DrawCircularOrbit(const FNovaSlateArguments& SlateArgs, const FVector2D& Origin, float Radius,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		FNovaSplineResults Results = DrawOrbit(SlateArgs, FNovaOrbitGeometry(Origin, Radius), PointsOfInterest, Style);
		for (const FVector2D& Point : Results.PointsOfInterest)
		{
			DrawPoint(SlateArgs, Point, Style.ColorInner);
		}
	}

	/** Draw a partial circular orbit around Origin of Radius, starting at Phase over AngularLength */
	void DrawPartialCircularOrbit(const FNovaSlateArguments& SlateArgs, const FVector2D& Origin, float Radius, float Phase, float InitialAngle, float AngularLength,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		FNovaSplineResults Results = DrawOrbit(SlateArgs, FNovaOrbitGeometry(Origin, Radius, Radius, Phase, InitialAngle, AngularLength, 0.0f), PointsOfInterest, Style);
		for (const FVector2D& Point : Results.PointsOfInterest)
		{
			DrawPoint(SlateArgs, Point, Style.ColorInner);
		}
	}

	/** Draw a Hohmann transfer orbit around Origin from RadiusA to RadiusB, starting at Phase */
	void DrawTransferOrbit(const FNovaSlateArguments& SlateArgs, const FVector2D& Origin, float RadiusA, float RadiusB, float Phase, float InitialAngle,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		float MajorAxis = 0.5f * (RadiusA + RadiusB);
		float MinorAxis = FMath::Sqrt(RadiusA * RadiusB);

		FNovaSplineResults Results = DrawOrbit(SlateArgs, FNovaOrbitGeometry(Origin, MajorAxis, MinorAxis, Phase, InitialAngle, 180, 0.5f * (RadiusB - RadiusA)), PointsOfInterest, Style);
		DrawPoint(SlateArgs, Results.InitialPosition, Style.ColorInner);
		DrawPoint(SlateArgs, Results.FinalPosition, Style.ColorInner);

		for (const FVector2D& Point : Results.PointsOfInterest)
		{
			DrawPoint(SlateArgs, Point, Style.ColorInner);
		}
	}

	int32 OnPaint(const FPaintArgs& PaintArgs, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		FNovaSlateArguments SlateArgs = FNovaSlateArguments(AllottedGeometry, OutDrawElements, LayerId);
		FNovaSplineStyle Style(FLinearColor::White);
		FVector2D Origin = FVector2D(500, 500);

		TArray<float> PointsOfInterest = { };

		DrawCircularOrbit(SlateArgs, Origin, 300, PointsOfInterest, Style);
		DrawCircularOrbit(SlateArgs, Origin, 450, PointsOfInterest, Style);
		DrawTransferOrbit(SlateArgs, Origin, 300, 450, 45, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Red));

		DrawTransferOrbit(SlateArgs, Origin, 300, 500, 90, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Green));
		DrawTransferOrbit(SlateArgs, Origin, 500, 450, 180 + 90, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Green));

		DrawTransferOrbit(SlateArgs, Origin, 300, 250, 135, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));
		DrawPartialCircularOrbit(SlateArgs, Origin, 250, 135 + 180, 0, 45, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));
		DrawTransferOrbit(SlateArgs, Origin, 250, 450, 135 + 180 + 45, 0, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));

		return SCompoundWidget::OnPaint(PaintArgs, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

};


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments()
		.Menu(InArgs._Menu)
	);

	// Structure
	ChildSlot
	[
		SNew(SHorizontalBox)

		// Main box
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Top)
		.AutoWidth()
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaNew(SNovaButton)
				.Text(LOCTEXT("TestJoin", "Join random session"))
				.HelpText(LOCTEXT("HelpTestJoin", "Join random session"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					MenuManager->GetPC()->TestJoin();
				}))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaNew(SNovaButton)
				.Text(LOCTEXT("LeaveStation", "Leave station"))
				.HelpText(LOCTEXT("HelpLeaveStation", "Leave station"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					const class UNovaArea* Orbit = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{D1D46588-494D-E081-ADE6-48AE0B010BBB}"));
					MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChangeArea(Orbit);
				}))
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
				{
					return MenuManager->GetPC() && MenuManager->GetPC()->GetLocalRole() == ROLE_Authority;
				})))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaNew(SNovaButton)
				.Text(LOCTEXT("GoToStation", "Go to station"))
				.HelpText(LOCTEXT("HelpGoToStation", "Go to station"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					const class UNovaArea* Station = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
					MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChangeArea(Station);
				}))
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
				{
					return MenuManager->GetPC() && MenuManager->GetPC()->GetLocalRole() == ROLE_Authority;
				})))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaAssignNew(UndockButton, SNovaButton)
				.Text(LOCTEXT("Undock", "Undock"))
				.HelpText(LOCTEXT("UndockHelp", "Undock from the station"))
				.OnClicked(this, &SNovaMainMenuFlight::OnUndock)
				.Enabled(this, &SNovaMainMenuFlight::IsUndockEnabled)
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaAssignNew(DockButton, SNovaButton)
				.Text(LOCTEXT("Dock", "Dock"))
				.HelpText(LOCTEXT("DockHelp", "Dock at the station"))
				.OnClicked(this, &SNovaMainMenuFlight::OnDock)
				.Enabled(this, &SNovaMainMenuFlight::IsDockEnabled)
			]
		]

		+ SHorizontalBox::Slot()
		[
			SAssignNew(MapRetainer, SRetainerWidget)
			[
				SNew(SNovaOrbitalMap)
			]
		]
	];

	// Setup retainer
	MapRetainer->SetTextureParameter(TEXT("UI"));
	MapRetainer->SetEffectMaterial(Theme.EffectMaterial);
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();

	GetSpacecraftPawn()->SetHighlightCompartment(INDEX_NONE);
}

void SNovaMainMenuFlight::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->TiltInput(Value);
	}
}

TSharedPtr<SNovaButton> SNovaMainMenuFlight::GetDefaultFocusButton() const
{
	if (IsUndockEnabled())
	{
		return UndockButton;
	}
	else
	{
		return DockButton;
	}
}


/*----------------------------------------------------
	Internals
----------------------------------------------------*/

ANovaSpacecraftPawn* SNovaMainMenuFlight::GetSpacecraftPawn() const
{
	return MenuManager->GetPC() ? MenuManager->GetPC()->GetSpacecraftPawn() : nullptr;
}

UNovaSpacecraftMovementComponent* SNovaMainMenuFlight::GetSpacecraftMovement() const
{
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	if (SpacecraftPawn)
	{
		return Cast<UNovaSpacecraftMovementComponent>(
			SpacecraftPawn->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
	}
	else
	{
		return nullptr;
	}
}


/*----------------------------------------------------
	Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->GetState() == ENovaMovementState::Docked;
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->GetState() == ENovaMovementState::Idle;
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

void SNovaMainMenuFlight::OnUndock()
{
	NCHECK(IsUndockEnabled());

	MenuManager->GetPC()->Undock();
}

void SNovaMainMenuFlight::OnDock()
{
	NCHECK(IsDockEnabled());

	MenuManager->GetPC()->Dock();
}


#undef LOCTEXT_NAMESPACE
