// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "Widgets/SCompoundWidget.h"

/** Info box color type */
enum class ENovaInfoBoxType : uint8
{
	None,
	Positive,
	Negative,
	Neutral
};

/** Info box class */
class SNovaInfoText : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaInfoText)
	{}

	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ATTRIBUTE(ENovaInfoBoxType, Type)
	SLATE_ATTRIBUTE(FText, Text)

	SLATE_END_ARGS()

public:

	SNovaInfoText() : DisplayedType(ENovaInfoBoxType::None), CurrentColor(0, 0, 0, 0), PreviousColor(0, 0, 0, 0), TimeSinceColorChange(0.0f)
	{}

	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		Type = InArgs._Type;

		// clang-format off
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(&Theme.White)
			.BorderBackgroundColor(this, &SNovaInfoText::GetColor)
			.Padding(Theme.ContentPadding)
			[
				SNew(SRichTextBlock)
				.Text(InArgs._Text)
				.TextStyle(&Theme.MainFont)
				.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
				+ SRichTextBlock::ImageDecorator()
			]
		];
		// clang-format on
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

		if (Type.IsSet() || Type.IsBound())
		{
			ENovaInfoBoxType DesiredType = Type.Get();

			if (DesiredType != ENovaInfoBoxType::None && DisplayedType == ENovaInfoBoxType::None)
			{
				CurrentColor = GetDesiredColor(DesiredType);

				DisplayedType        = DesiredType;
				TimeSinceColorChange = 0.0f;
				PreviousColor        = CurrentColor;
			}
			else if (DesiredType != DisplayedType)
			{
				FLinearColor DesiredColor = GetDesiredColor(DesiredType);

				float Alpha = FMath::Clamp(TimeSinceColorChange / ENovaUIConstants::FadeDurationShort, 0.0f, 1.0f);
				TimeSinceColorChange += DeltaTime;

				CurrentColor = FMath::InterpEaseInOut(PreviousColor, DesiredColor, Alpha, ENovaUIConstants::EaseStandard);

				if (Alpha >= 1.0f)
				{
					DisplayedType        = DesiredType;
					TimeSinceColorChange = 0.0f;
					PreviousColor        = CurrentColor;
				}
			}
		}
	}

protected:

	FSlateColor GetColor() const
	{
		return CurrentColor;
	}

	FLinearColor GetDesiredColor(ENovaInfoBoxType DesiredType) const
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
		switch (DesiredType)
		{
			case ENovaInfoBoxType::Positive:
				return Theme.PositiveColor;
			case ENovaInfoBoxType::Negative:
				return Theme.NegativeColor;
			case ENovaInfoBoxType::Neutral:
				return Theme.NeutralColor;
		}

		return FLinearColor(0, 0, 0, 0);
	}

protected:

	// General state
	TAttribute<ENovaInfoBoxType> Type;
	ENovaInfoBoxType             DisplayedType;
	FLinearColor                 CurrentColor;
	FLinearColor                 PreviousColor;
	float                        TimeSinceColorChange;
};
