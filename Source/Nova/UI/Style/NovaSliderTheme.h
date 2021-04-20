// Nova project - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NovaSliderTheme.generated.h"

USTRUCT()
struct FNovaSliderTheme : public FSlateWidgetStyle
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

	static const FNovaSliderTheme& GetDefault()
	{
		static FNovaSliderTheme Default;
		return Default;
	}

	void GetResources(TArray<const FSlateBrush*>& OutBrushes) const
	{}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	UPROPERTY(EditDefaultsOnly, Category = Slider) FSliderStyle SliderStyle;
};

/*----------------------------------------------------
    Wrapper classes
----------------------------------------------------*/

UCLASS()
class UNovaSliderThemeContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:
	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Nova, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FNovaSliderTheme Style;
};
