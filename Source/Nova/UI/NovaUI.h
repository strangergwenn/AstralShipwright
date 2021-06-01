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

/** Button-width class */
class SNovaButtonLayout : public SBox
{
	SLATE_BEGIN_ARGS(SNovaButtonLayout) : _Theme("DefaultButton"), _Size("DefaultButtonSize")
	{}

	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(InArgs._Theme);
		const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(InArgs._Size);

		// clang-format off
		SBox::Construct(SBox::FArguments()
			.WidthOverride(Size.Width)
			.Padding(Theme.HoverAnimationPadding + 1)
			[
				InArgs._Content.Widget
			]
		);
		// clang-format on
	}
};
