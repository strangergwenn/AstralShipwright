// Nova project - Gwennaël Arbona

#include "NovaEventDisplay.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "NovaEventDisplay"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaEventDisplay::SNovaEventDisplay()
{}

void SNovaEventDisplay::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Settings
	MenuManager = InArgs._MenuManager;

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0, 90))
	[
		SNew(SBorder)
		.BorderImage(&Theme.MainMenuGenericBorder)
		.Padding(2)
		.BorderBackgroundColor(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([&]()
		{
			return 1.25f * MenuManager->GetHighlightColor();
		})))
		[
			SNew(SBackgroundBlur)
			.BlurRadius(Theme.BlurRadius)
			.BlurStrength(Theme.BlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuGenericBackground)
				.Padding(Theme.ContentPadding)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.HeadingFont)
						.Text(LOCTEXT("Test", "Imminent maneuver").ToUpper())
						.ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([&]()
						{
							return 1.25f * MenuManager->GetHighlightColor();
						})))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(LOCTEXT("TestTime", "15 seconds left"))
						.ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([&]()
						{
							return 1.25f * MenuManager->GetHighlightColor();
						})))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(LOCTEXT("TestSub", "Spacecraft is not cleared to maneuver").ToUpper())
						.ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateLambda([&]()
						{
							return 1.25f * MenuManager->GetHighlightColor();
						})))
					]
				]
			]
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaEventDisplay::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

#undef LOCTEXT_NAMESPACE
