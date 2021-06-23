// Nova project - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NovaMainTheme.generated.h"

USTRUCT()
struct FNovaMainTheme : public FSlateWidgetStyle
{
	GENERATED_BODY()

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	static const FName TypeName;
	const FName        GetTypeName() const override
	{
		return TypeName;
	}

	static const FNovaMainTheme& GetDefault()
	{
		static FNovaMainTheme Default;
		return Default;
	}

	void GetResources(TArray<const FSlateBrush*>& OutBrushes) const
	{
		OutBrushes.Add(&MainMenuBackground);
		OutBrushes.Add(&MainMenuGenericBackground);
		OutBrushes.Add(&MainMenuDarkBackground);
		OutBrushes.Add(&MainMenuGenericBorder);
		OutBrushes.Add(&MainMenuManipulator);
		OutBrushes.Add(&TableHeaderBackground);
		OutBrushes.Add(&TableOddBackground);
		OutBrushes.Add(&TableEvenBackground);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// General
	UPROPERTY(EditDefaultsOnly, Category = General) FMargin ContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = General) FMargin VerticalContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = General) float BlurRadius;
	UPROPERTY(EditDefaultsOnly, Category = General) float BlurStrength;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor PositiveColor;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor NegativeColor;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor NeutralColor;
	UPROPERTY(EditDefaultsOnly, Category = General) int32 NotificationDisplayWidth;

	// Fonts
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle TitleFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle NotificationFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle HeadingFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle MainFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle InfoFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle KeyFont;

	// Main
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuGenericBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuPatternedBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuDarkBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuGenericBorder;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuManipulator;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush TableHeaderBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush TableOddBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush TableEvenBackground;

	// Widget styles
	UPROPERTY(EditDefaultsOnly, Category = Widgets) FSliderStyle SliderStyle;
	UPROPERTY(EditDefaultsOnly, Category = Widgets) FScrollBoxStyle ScrollBoxStyle;
	UPROPERTY(EditDefaultsOnly, Category = Widgets) FProgressBarStyle ProgressBarStyle;
};

/*----------------------------------------------------
    Wrapper class
----------------------------------------------------*/

UCLASS()
class UNovaMainThemeContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:
	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Nova, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FNovaMainTheme Style;
};
