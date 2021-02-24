// Nova project - Gwennaël Arbona

#include "NovaLoadingScreen.h"
#include "Nova/UI/NovaUI.h"
#include "Nova/Player/NovaGameViewportClient.h"
#include "Nova/Nova.h"

#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Materials/MaterialInstanceDynamic.h"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaLoadingScreen::Construct(const FArguments& InArgs)
{
	// Get args
	AnimatedMaterialInstance = InArgs._Material;
	NCHECK(AnimatedMaterialInstance);
	uint32_t Width  = InArgs._Settings->Width;
	uint32_t Height = InArgs._Settings->Height;

	// Build the top loading brush
	LoadingScreenBrush = FDeferredCleanupSlateBrush::CreateBrush(AnimatedMaterialInstance, FVector2D(Width, Height));
	NCHECK(LoadingScreenBrush.IsValid());

	// clang-format off
	ChildSlot
	[
		SNew(SScaleBox)
		.IgnoreInheritedScale(true)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			// Background
			SNew(SOverlay)
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(FNovaStyleSet::GetBrush("Common/SB_Black"))
				.BorderBackgroundColor(this, &SNovaLoadingScreen::GetColor)
			]

			// Animated brush
			+ SOverlay::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.WidthOverride(Width)
				.HeightOverride(Height)
				[
					SNew(SBorder)
					.BorderImage(LoadingScreenBrush->GetSlateBrush())
					.BorderBackgroundColor(this, &SNovaLoadingScreen::GetColor)
				]
			]
		]
	];
	// clang-format on

	// Defaults
	SetVisibility((EVisibility::HitTestInvisible));
	CurrentAlpha = 1;
}

void SNovaLoadingScreen::SetFadeAlpha(float Alpha)
{
	CurrentAlpha = Alpha;
}

FSlateColor SNovaLoadingScreen::GetColor() const
{
	return FLinearColor(1, 1, 1, CurrentAlpha);
}

float SNovaLoadingScreen::GetCurrentTime() const
{
	return LoadingScreenTime;
}

int32 SNovaLoadingScreen::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	while (AnimatedMaterialInstance->IsUnreachable())
	{
		FPlatformProcess::Sleep(0.001f);
	}
	NCHECK(!AnimatedMaterialInstance->IsUnreachable());

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

void SNovaLoadingScreen::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	if (AnimatedMaterialInstance)
	{
		AnimatedMaterialInstance->SetScalarParameterValue("Time", LoadingScreenTime);
	}

	LoadingScreenTime += DeltaTime;
}
