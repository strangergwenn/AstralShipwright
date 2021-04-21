// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/NovaUITypes.h"
#include "Widgets/SCompoundWidget.h"

/** Simple widget that can be displayed with fade-in and fade-out */
template <bool FadeOut = true>
class SNovaFadingWidget : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaFadingWidget)
	{}

	SLATE_ARGUMENT(float, FadeDuration)
	SLATE_ARGUMENT(float, DisplayDuration)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	/*----------------------------------------------------
	    Slate interface
	----------------------------------------------------*/

public:
	SNovaFadingWidget() : CurrentFadeTime(0), CurrentDisplayTime(0), CurrentAlpha(0)
	{}

	void Construct(const FArguments& InArgs)
	{
		// Settings
		FadeDuration    = InArgs._FadeDuration;
		DisplayDuration = InArgs._DisplayDuration;

		// clang-format off
		ChildSlot
		[
			InArgs._Content.Widget
		];
		// clang-format on

		// Defaults
		CurrentDisplayTime = DisplayDuration;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		// Update time
		if ((FadeOut && CurrentDisplayTime > DisplayDuration) || IsDirty())
		{
			CurrentFadeTime -= DeltaTime;
		}
		else
		{
			CurrentFadeTime += DeltaTime;
		}
		CurrentFadeTime = FMath::Clamp(CurrentFadeTime, 0.0f, FadeDuration);
		CurrentAlpha    = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENovaUIConstants::EaseStandard);

		// Update when invisible
		if (CurrentFadeTime <= 0 && IsDirty())
		{
			CurrentDisplayTime = 0;

			OnUpdate();
		}
		else
		{
			CurrentDisplayTime += DeltaTime;
		}
	}

	/*----------------------------------------------------
	    Virtual interface
	----------------------------------------------------*/

protected:
	/** Return true to request a widget update */
	virtual bool IsDirty() const
	{
		return false;
	}

	/** Update the widget's contents */
	virtual void OnUpdate()
	{}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

public:
	FLinearColor GetLinearColor() const
	{
		// Hack for proper smoothing of background blurs
		return FLinearColor(1.0f, 1.0f, 1.0f, CurrentAlpha > 0.1f ? CurrentAlpha : 0.0f);
	}

	FSlateColor GetSlateColor() const
	{
		return GetLinearColor();
	}

	FSlateColor GetTextColor(const FTextBlockStyle& TextStyle) const
	{
		FLinearColor Color = TextStyle.ColorAndOpacity.GetSpecifiedColor();
		Color.A *= CurrentAlpha;

		return Color;
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
	// Settings
	float FadeDuration;
	float DisplayDuration;

	// Current state
	float CurrentFadeTime;
	float CurrentDisplayTime;
	float CurrentAlpha;
};
