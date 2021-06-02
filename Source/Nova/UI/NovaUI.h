// Nova project - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"

#include "Style/NovaStyleSet.h"
#include "Style/NovaMainTheme.h"

/*----------------------------------------------------
    Slate widget creation macros
    Implement NewNovaButton in the user class
----------------------------------------------------*/

/** Equivalent to SNew, automatically store the button into FocusableButtons */
#define SNovaNew(WidgetType, ...)                                                                                      \
	NewNovaButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), false) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SNew, automatically store the button into FocusableButtons and set as default focus */
#define SNovaDefaultNew(WidgetType, ...)                                                                              \
	NewNovaButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), true) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SAssignNew, automatically store the button into FocusableButtons */
#define SNovaAssignNew(ExposeAs, WidgetType, ...)                                                                                       \
	NewNovaButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), false).Expose(ExposeAs) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SAssignNew, automatically store the button into FocusableButtons and set as default focus */
#define SNovaDefaultAssignNew(ExposeAs, WidgetType, ...)                                                                               \
	NewNovaButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), true).Expose(ExposeAs) <<= \
		TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/*----------------------------------------------------
    Menu types
----------------------------------------------------*/

/** Game menu interface */
class INovaGameMenu
{
public:
	virtual void UpdateGameObjects(){};
};

/** Button layout imposter class */
class SNovaButtonLayout : public SBox
{
	SLATE_BEGIN_ARGS(SNovaButtonLayout) : _Theme("DefaultButton"), _Size(NAME_None), _WidthOnly(true)
	{}

	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)
	SLATE_ARGUMENT(bool, WidthOnly)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		// Build padding
		const FNovaButtonTheme& Theme   = FNovaStyleSet::GetButtonTheme(InArgs._Theme);
		FMargin                 Padding = Theme.HoverAnimationPadding + 1;
		if (InArgs._WidthOnly)
		{
			Padding.Top    = 0;
			Padding.Bottom = 0;
		}

		// clang-format off
		if (InArgs._Size != NAME_None)
		{
			const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(InArgs._Size);

			SBox::Construct(SBox::FArguments()
				.WidthOverride(Size.Width)
				.Padding(Padding)
				[
					InArgs._Content.Widget
				]
			);
		}
		else
		{
			SBox::Construct(SBox::FArguments()
				.Padding(Padding)
				[
					InArgs._Content.Widget
				]
			);
		}
		// clang-format on
	}
};

/** Info box color type */
enum class ENovaInfoBoxType : uint8
{
	Positive,
	Negative,
	Neutral
};

/** Info box class */
class SNovaInfoText : public SBorder
{
	SLATE_BEGIN_ARGS(SNovaInfoText)
	{}

	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ATTRIBUTE(ENovaInfoBoxType, Type)
	SLATE_ATTRIBUTE(FText, Text)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		Type = InArgs._Type;

		// clang-format off
		SBorder::Construct(SBorder::FArguments()
			.BorderImage(FNovaStyleSet::GetBrush("Common/SB_White"))
			.BorderBackgroundColor(this, &SNovaInfoText::GetColor)
			.Padding(Theme.ContentPadding)
			[
				SNew(SRichTextBlock)
				.Text(InArgs._Text)
				.TextStyle(&Theme.MainFont)
				.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
				+ SRichTextBlock::ImageDecorator()
			]
		);
		// clang-format on
	}

protected:
	FSlateColor GetColor() const
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
		switch (Type.Get())
		{
			case ENovaInfoBoxType::Positive:
				return Theme.PositiveColor;
			case ENovaInfoBoxType::Negative:
				return Theme.NegativeColor;
			case ENovaInfoBoxType::Neutral:
				return Theme.NeutralColor;
		}

		return FSlateColor();
	}

protected:
	TAttribute<ENovaInfoBoxType> Type;
};
