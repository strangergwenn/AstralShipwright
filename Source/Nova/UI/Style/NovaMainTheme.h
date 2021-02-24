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
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Main
	UPROPERTY(EditDefaultsOnly, Category = Main) FSlateBrush MainMenuBackground;
	UPROPERTY(EditDefaultsOnly, Category = Main) FSlateBrush MainMenuGenericBackground;
	UPROPERTY(EditDefaultsOnly, Category = Main) FSlateBrush MainMenuGenericBorder;
	UPROPERTY(EditDefaultsOnly, Category = Main) FMargin ContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = Main) FMargin VerticalContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = Main) FScrollBoxStyle ScrollBoxStyle;
	UPROPERTY(EditDefaultsOnly, Category = Main) FProgressBarStyle ProgressBarStyle;
	UPROPERTY(EditDefaultsOnly, Category = Main) float BlurRadius;
	UPROPERTY(EditDefaultsOnly, Category = Main) float BlurStrength;
	UPROPERTY(EditDefaultsOnly, Category = Main) class UMaterialInterface* EffectMaterial;

	// Fonts
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle TitleFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle SubtitleFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle MainFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle InfoFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle HUDFont;
	UPROPERTY(EditDefaultsOnly, Category = Font) FTextBlockStyle KeyFont;

	// HUD
	UPROPERTY(EditDefaultsOnly, Category = HUD) FMargin HudInteractionMargin;
	UPROPERTY(EditDefaultsOnly, Category = HUD) FMargin HudProgressBarPadding;
	UPROPERTY(EditDefaultsOnly, Category = HUD) int32 HudProgressBarWidth;

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
