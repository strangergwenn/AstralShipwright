// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"

/** Abstract table class with value comparison */
class SNovaTable : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaTable) : _Width(FOptionalSize())
	{}

	SLATE_ARGUMENT(FText, Title)
	SLATE_ARGUMENT(FOptionalSize, Width)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		Width = InArgs._Width;

		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// clang-format off
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(InArgs._Width)
			[
				SAssignNew(TableBox, SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(InArgs._Title)
					.TextStyle(&Theme.HeadingFont)
				]
			]
		];
		// clang-format on
	}

	/** Add a table header */
	void AddHeader(FText Label, FText Details = FText())
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// clang-format off
		TableBox->AddSlot()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SBorder)
			.BorderImage(&Theme.TableHeaderBackground)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				[
					SNew(SRichTextBlock)
					.Text(Label)
					.TextStyle(&Theme.InfoFont)
					.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
					+ SRichTextBlock::ImageDecorator()
				]

				+ SHorizontalBox::Slot()
				[
					SNew(SRichTextBlock)
					.Text(Details)
					.TextStyle(&Theme.InfoFont)
					.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
					+ SRichTextBlock::ImageDecorator()
				]
			]
		];
		// clang-format on

		EvenRow = false;
	}

	/** Add a numerical table entry */
	void AddEntry(FText Label, float CurrentValue, float PreviousValue = -1, FText Unit = FText())
	{
		// Fetch the style data
		const FNovaMainTheme&    Theme = FNovaStyleSet::GetMainTheme();
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits            = 1;
		FNumberFormattingOptions DifferenceOptions = Options;
		DifferenceOptions.AlwaysSign               = true;

		// Build the comparison data
		FLinearColor Color = FLinearColor::White;
		FText        Text  = FText::FromString(FText::AsNumber(CurrentValue, &Options).ToString() + " " + Unit.ToString());
		if (PreviousValue >= 0 && CurrentValue != PreviousValue)
		{
			Color = CurrentValue > PreviousValue ? Theme.PositiveColor : Theme.NegativeColor;
			Text  = FText::FromString(
                Text.ToString() + " (" + FText::AsNumber(CurrentValue - PreviousValue, &DifferenceOptions).ToString() + ")");
		}

		AddRow(Label, Text, Color);
	}

	/** Add a textual table entry */
	void AddEntry(FText Label, FText CurrentText, FText PreviousText = FText(), FText Unit = FText())
	{
		// Fetch the style data
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// Build the comparison data
		FLinearColor Color = FLinearColor::White;
		if (!PreviousText.IsEmpty() && !CurrentText.EqualTo(PreviousText))
		{
			Color = Theme.PositiveColor;
		}

		AddRow(Label, CurrentText, Color);
	}

protected:
	/** Add a generic row to the table */
	void AddRow(FText Label, FText Value, FLinearColor Color = FLinearColor::White)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// clang-format off
		TableBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(EvenRow ? &Theme.TableEvenBackground : &Theme.TableOddBackground)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				[
					SNew(SRichTextBlock)
					.Text(Label)
					.TextStyle(&Theme.MainFont)
					.WrapTextAt(Width.Get() / 2)
					.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
					+ SRichTextBlock::ImageDecorator()
				]

				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(Value)
					.TextStyle(&Theme.MainFont)
					.ColorAndOpacity(Color)
				]
			]
		];
		// clang-format on

		EvenRow = !EvenRow;
	}

protected:
	TSharedPtr<SVerticalBox> TableBox;
	bool                     EvenRow;
	FOptionalSize            Width;
};

#undef LOCTEXT_NAMESPACE
