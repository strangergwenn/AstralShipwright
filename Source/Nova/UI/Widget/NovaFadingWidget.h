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

	SLATE_BEGIN_ARGS(SNovaFadingWidget) : _FadeDuration(ENovaUIConstants::FadeDurationShort), _ColorAndOpacity(FLinearColor::White)
	{}

	SLATE_ARGUMENT(float, FadeDuration)
	SLATE_ARGUMENT(float, DisplayDuration)
	SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)
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
		FadeDuration           = InArgs._FadeDuration;
		DisplayDuration        = InArgs._DisplayDuration;
		DesiredColorAndOpacity = InArgs._ColorAndOpacity;

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
	float GetCurrentAlpha() const
	{
		return CurrentAlpha;
	}

	FLinearColor GetLinearColor() const
	{
		FLinearColor Color = DesiredColorAndOpacity.Get();
		Color.A *= CurrentAlpha > 0.1f ? CurrentAlpha : 0.0f;
		return Color;
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
	float                    FadeDuration;
	float                    DisplayDuration;
	TAttribute<FLinearColor> DesiredColorAndOpacity;

	// Current state
	float CurrentFadeTime;
	float CurrentDisplayTime;
	float CurrentAlpha;
};

/** Image callback */
DECLARE_DELEGATE_RetVal(const FSlateBrush*, FNovaImageGetter);

/** Simple SImage analog that fades smoothly when the image changes */
class SNovaImage : public SNovaFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaImage) : _ColorAndOpacity(FLinearColor::White)
	{}

	SLATE_ARGUMENT(FNovaImageGetter, Image)
	SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		Getter = InArgs._Image;

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
			.FadeDuration(ENovaUIConstants::FadeDurationShort)
			.DisplayDuration(4.0f)
			.ColorAndOpacity(InArgs._ColorAndOpacity)
		);

		ChildSlot
		[
			SNew(SImage)
			.Image(this, &SNovaImage::GetImage)
			.ColorAndOpacity(this, &SNovaFadingWidget::GetSlateColor)

		];
		// clang-format on
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
	{
		SNovaFadingWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		if (Getter.IsBound())
		{
			DesiredImage = Getter.Execute();
		}
	}

	virtual bool IsDirty() const override
	{
		return DesiredImage != CurrentImage;
	}

	virtual void OnUpdate() override
	{
		CurrentImage = DesiredImage;
	}

	const FSlateBrush* GetImage() const
	{
		return CurrentImage;
	}

private:
	const FSlateBrush* DesiredImage;
	const FSlateBrush* CurrentImage;
	FNovaImageGetter   Getter;
};

/** Text callback */
DECLARE_DELEGATE_RetVal(FText, FNovaTextGetter);

/** Simple STextBlock analog that fades smoothly when the text changes */
class SNovaText : public SNovaFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaText) : _AutoWrapText(false)
	{}

	SLATE_ARGUMENT(FNovaTextGetter, Text)
	SLATE_STYLE_ARGUMENT(FTextBlockStyle, TextStyle)
	SLATE_ATTRIBUTE(float, WrapTextAt)
	SLATE_ATTRIBUTE(bool, AutoWrapText)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		Getter = InArgs._Text;

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
			.FadeDuration(ENovaUIConstants::FadeDurationShort)
			.DisplayDuration(4.0f)
		);

		ChildSlot
		[
			SNew(STextBlock)
			.Text(this, &SNovaText::GetText)
			.TextStyle(InArgs._TextStyle)
			.WrapTextAt(InArgs._WrapTextAt)
				.AutoWrapText(InArgs._AutoWrapText)
			.ColorAndOpacity(this, &SNovaFadingWidget::GetSlateColor)

		];
		// clang-format on
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
	{
		SNovaFadingWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		if (Getter.IsBound())
		{
			DesiredText = Getter.Execute();
		}
	}

	virtual bool IsDirty() const override
	{
		return !DesiredText.EqualTo(CurrentText);
	}

	virtual void OnUpdate() override
	{
		CurrentText = DesiredText;
	}

	FText GetText() const
	{
		return CurrentText;
	}

protected:
	FText           DesiredText;
	FText           CurrentText;
	FNovaTextGetter Getter;
};

/** Simple SRichTextBlock analog that fades smoothly when the text changes */
class SNovaRichText : public SNovaText
{
	SLATE_BEGIN_ARGS(SNovaRichText) : _AutoWrapText(false)
	{}

	SLATE_ARGUMENT(FNovaTextGetter, Text)
	SLATE_STYLE_ARGUMENT(FTextBlockStyle, TextStyle)
	SLATE_ATTRIBUTE(float, WrapTextAt)
	SLATE_ATTRIBUTE(bool, AutoWrapText)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		Getter = InArgs._Text;

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
			.FadeDuration(ENovaUIConstants::FadeDurationShort)
			.DisplayDuration(4.0f)
		);

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(new FSlateNoResource)
			.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
			.Padding(0)
			[
				SNew(SRichTextBlock)
				.Text(this, &SNovaText::GetText)
				.TextStyle(InArgs._TextStyle)
				.WrapTextAt(InArgs._WrapTextAt)
				.AutoWrapText(InArgs._AutoWrapText)
				.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
				+ SRichTextBlock::ImageDecorator()
			]

		];
		// clang-format on
	}
};
