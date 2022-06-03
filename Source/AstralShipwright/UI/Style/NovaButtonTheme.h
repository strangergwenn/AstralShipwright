// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NovaButtonTheme.generated.h"

/** Theme structure storing visual elements of a button */
USTRUCT()
struct FNovaButtonTheme : public FSlateWidgetStyle
{
	GENERATED_BODY()

	FNovaButtonTheme() : DisabledColor(FLinearColor::Gray), Centered(false), WrapMargin(48), AnimationDuration(0.2f)
	{}

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	static const FName TypeName;
	const FName        GetTypeName() const override
	{
		return TypeName;
	}

	static const FNovaButtonTheme& GetDefault()
	{
		static FNovaButtonTheme Default;
		return Default;
	}

	void GetResources(TArray<const FSlateBrush*>& OutBrushes) const
	{
		OutBrushes.Add(&Border);
		OutBrushes.Add(&Background);
		OutBrushes.Add(&ListOn);
		OutBrushes.Add(&ListOff);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	UPROPERTY(EditDefaultsOnly, Category = Button) FTextBlockStyle Font;
	UPROPERTY(EditDefaultsOnly, Category = Button) FMargin IconPadding;
	UPROPERTY(EditDefaultsOnly, Category = Button) FMargin HoverAnimationPadding;
	UPROPERTY(EditDefaultsOnly, Category = Button) FSlateBrush Border;
	UPROPERTY(EditDefaultsOnly, Category = Button) FSlateBrush Background;
	UPROPERTY(EditDefaultsOnly, Category = Button) FSlateBrush ListOn;
	UPROPERTY(EditDefaultsOnly, Category = Button) FSlateBrush ListOff;
	UPROPERTY(EditDefaultsOnly, Category = Button) FLinearColor DisabledColor;
	UPROPERTY(EditDefaultsOnly, Category = Button) bool Centered;
	UPROPERTY(EditDefaultsOnly, Category = Button) int32 WrapMargin;
	UPROPERTY(EditDefaultsOnly, Category = Button) float AnimationDuration;
	UPROPERTY(EditDefaultsOnly, Category = Button) FSlateSound ClickSound;
};

/** Theme structure storing dimensional elements of a button */
USTRUCT()
struct FNovaButtonSize : public FSlateWidgetStyle
{
	GENERATED_BODY()

	FNovaButtonSize() : Width(0), Height(0), DisabledAnimationSize(FVector2D::ZeroVector), UserAnimationSize(FVector2D::ZeroVector)
	{}

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	static const FName TypeName;
	const FName        GetTypeName() const override
	{
		return TypeName;
	}

	static const FNovaButtonSize& GetDefault()
	{
		static FNovaButtonSize Default;
		return Default;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	UPROPERTY(EditDefaultsOnly, Category = Button) int32 Width;
	UPROPERTY(EditDefaultsOnly, Category = Button) int32 Height;
	UPROPERTY(EditDefaultsOnly, Category = Button) FVector2D DisabledAnimationSize;
	UPROPERTY(EditDefaultsOnly, Category = Button) FVector2D UserAnimationSize;
};

/*----------------------------------------------------
    Wrapper classes
----------------------------------------------------*/

UCLASS()
class UNovaButtonThemeContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Nova, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FNovaButtonTheme Style;
};

UCLASS()
class UNovaButtonSizeContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Nova, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FNovaButtonSize Style;
};
