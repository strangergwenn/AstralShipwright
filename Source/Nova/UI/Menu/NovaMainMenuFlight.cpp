// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaDestination.h"
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
			, Angle(0)
			, AngularLength(360)
			, TransferOffset(0)
		{}

		FNovaOrbitGeometry(const FVector2D& Orig, float W, float H, float A)
			: FNovaOrbitGeometry(Orig, W)
		{
			Height = H;
			Angle = A;
		}

		FNovaOrbitGeometry(const FVector2D& Orig, float W, float H, float A, float Partial, float Offset)
			: FNovaOrbitGeometry(Orig, W, H, A)
		{
			AngularLength = Partial;
			TransferOffset = Offset;
		}

		FVector2D Origin;
		float Width;
		float Height;
		float Angle;

		float AngularLength;
		float TransferOffset;
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
		const float InvAlpha = 1.0f - Alpha;

		const FVector2D L = InvAlpha * P0 + Alpha * P1;
		const FVector2D M = InvAlpha * P1 + Alpha * P2;
		const FVector2D N = InvAlpha * P2 + Alpha * P3;

		const FVector2D P = InvAlpha * L + Alpha * M;
		const FVector2D Q = InvAlpha * M + Alpha * N;

		return InvAlpha * P + Alpha * Q;
	};

	/** Draw an orbit or partial orbit */
	FNovaSplineResults DrawOrbit(const FNovaSlateArguments& SlateArgs, const FNovaOrbitGeometry& Orbit, const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		FNovaSplineResults Results;

		for (int32 SplineIndex = 0; SplineIndex < 4; SplineIndex++)
		{
			// Compute how much of an angle is left
			float RemainingAngle = Orbit.AngularLength - SplineIndex * 90.0f;
			if (RemainingAngle <= 0.0f)
			{
				break;
			}
			RemainingAngle = FMath::Min(RemainingAngle, 90.0f);

			// Define the control points for a 90° segment of the orbit
			FVector2D BezierPoints[4];
			float CircleRatio = (RemainingAngle / 90.0f) * 0.55228475f;
			BezierPoints[0] = FVector2D(Orbit.Width, 0);
			BezierPoints[3] = FVector2D(0, -Orbit.Height).GetRotated(90 - RemainingAngle);
			BezierPoints[1] = BezierPoints[0] - FVector2D(0, CircleRatio * Orbit.Height);
			BezierPoints[2] = BezierPoints[3] + FVector2D(0, CircleRatio * Orbit.Width).GetRotated(-RemainingAngle);

			// Mirror and rotate the spline control points
			auto Transform = [Orbit, RemainingAngle, SplineIndex](const FVector2D& Point)
			{
				FVector2D Inversion[4];
				Inversion[0] = FVector2D(1, 1);
				Inversion[1] = FVector2D(-1, 1);
				Inversion[2] = FVector2D(-1, -1);
				Inversion[3] = FVector2D(1, -1);

				float Angle = -Orbit.Angle;
				if (RemainingAngle < 90 && SplineIndex % 2 == 1)
				{
					Angle += RemainingAngle;
				}

				return Orbit.Origin
					+ FVector2D(-Orbit.TransferOffset, 0).GetRotated(Angle)
					+ (Point * Inversion[SplineIndex]).GetRotated(Angle);
			};

			const FVector2D P0 = Transform(BezierPoints[0]);
			const FVector2D P1 = Transform(BezierPoints[1]);
			const FVector2D P2 = Transform(BezierPoints[2]);
			const FVector2D P3 = Transform(BezierPoints[3]);

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

			// Process special points
			for (float Point : PointsOfInterest)
			{
				const float ScaledPoint = 4.0f * (Point / 360.0f);
				if (ScaledPoint >= SplineIndex && ScaledPoint < SplineIndex + 1)
				{
					float CorrectedPoint = FMath::Fmod(ScaledPoint, 1.0f);
					CorrectedPoint = SplineIndex % 2 ? 1.0f - CorrectedPoint : CorrectedPoint;
					Results.PointsOfInterest.Add(DeCasteljauInterp(P0, P1, P2, P3, CorrectedPoint));
				}
			}

			// Report the initial and final positions
			if (SplineIndex == 0)
			{
				Results.InitialPosition = P0;
			}
			else
			{
				Results.FinalPosition = P0;
			}
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

	/** Draw a partial circular orbit around Origin of Radius, starting at Angle over AngularLength */
	void DrawPartialCircularOrbit(const FNovaSlateArguments& SlateArgs, const FVector2D& Origin, float Radius, float Angle, float AngularLength,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		FNovaSplineResults Results = DrawOrbit(SlateArgs, FNovaOrbitGeometry(Origin, Radius, Radius, Angle, AngularLength, 0.0f), PointsOfInterest, Style);
		for (const FVector2D& Point : Results.PointsOfInterest)
		{
			DrawPoint(SlateArgs, Point, Style.ColorInner);
		}
	}

	/** Draw a Hohmann transfer orbit around Origin from RadiusA to RadiusB, starting at Angle */
	void DrawTransferOrbit(const FNovaSlateArguments& SlateArgs, const FVector2D& Origin, float RadiusA, float RadiusB, float Angle,
		const TArray<float>& PointsOfInterest, const FNovaSplineStyle& Style) const
	{
		float MajorAxis = 0.5f * (RadiusA + RadiusB);
		float MinorAxis = FMath::Sqrt(RadiusA * RadiusB);

		FNovaSplineResults Results = DrawOrbit(SlateArgs, FNovaOrbitGeometry(Origin, MajorAxis, MinorAxis, Angle, 180, 0.5f * (RadiusB - RadiusA)), PointsOfInterest, Style);
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
		//TArray<float> PointsOfInterest = { 0, 45, 90, 180 };

		DrawCircularOrbit(SlateArgs, Origin, 300, PointsOfInterest, Style);
		DrawCircularOrbit(SlateArgs, Origin, 450, PointsOfInterest, Style);
		DrawTransferOrbit(SlateArgs, Origin, 300, 450, 45, PointsOfInterest, FNovaSplineStyle(FLinearColor::Red));

		DrawTransferOrbit(SlateArgs, Origin, 300, 500, 90, PointsOfInterest, FNovaSplineStyle(FLinearColor::Green));
		DrawTransferOrbit(SlateArgs, Origin, 500, 450, 180 + 90, PointsOfInterest, FNovaSplineStyle(FLinearColor::Green));

		DrawTransferOrbit(SlateArgs, Origin, 300, 250, 135, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));
		DrawPartialCircularOrbit(SlateArgs, Origin, 250, 135 + 180, 45, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));
		DrawTransferOrbit(SlateArgs, Origin, 250, 450, 135 + 180 + 45, PointsOfInterest, FNovaSplineStyle(FLinearColor::Blue));

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
					const class UNovaDestination* Orbit = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaDestination>(FGuid("{D1D46588-494D-E081-ADE6-48AE0B010BBB}"));
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
					const class UNovaDestination* Station = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaDestination>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
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
