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
		OutBrushes.Add(&MainMenuGenericBorder);
		OutBrushes.Add(&MainMenuManipulator);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Main
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuGenericBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuGenericBorder;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuManipulator;

	// General
	UPROPERTY(EditDefaultsOnly, Category = General) FMargin ContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = General) FMargin VerticalContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = General) FScrollBoxStyle ScrollBoxStyle;
	UPROPERTY(EditDefaultsOnly, Category = General) FProgressBarStyle ProgressBarStyle;
	UPROPERTY(EditDefaultsOnly, Category = General) float BlurRadius;
	UPROPERTY(EditDefaultsOnly, Category = General) float BlurStrength;

	// Fonts
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle TitleFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle SubtitleFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle MainFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle InfoFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle KeyFont;

	// Notifications
	UPROPERTY(EditDefaultsOnly, Category = Notification) int32 NotificationDisplayHeight;
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
