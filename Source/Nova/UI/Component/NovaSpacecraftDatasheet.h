// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"

#define LOCTEXT_NAMESPACE "SNovaSpacecraftDatasheet"

/** Spacecraft datasheet class */
class SNovaSpacecraftDatasheet : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaSpacecraftDatasheet) : _TargetSpacecraft(), _ComparisonSpacecraft(nullptr)
	{}

	SLATE_ARGUMENT(FText, Title)
	SLATE_ARGUMENT(FNovaSpacecraft, TargetSpacecraft)
	SLATE_ARGUMENT(const FNovaSpacecraft*, ComparisonSpacecraft)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		// Fetch the required data
		const FNovaMainTheme&                   Theme                       = FNovaStyleSet::GetMainTheme();
		const FNovaSpacecraft&                  Target                      = InArgs._TargetSpacecraft;
		const FNovaSpacecraft*                  Comparison                  = InArgs._ComparisonSpacecraft;
		const FNovaSpacecraftPropulsionMetrics& TargetPropulsionMetrics     = Target.GetPropulsionMetrics();
		const FNovaSpacecraftPropulsionMetrics* ComparisonPropulsionMetrics = Comparison ? &Comparison->GetPropulsionMetrics() : nullptr;

		// clang-format off
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(500)
			[
				SAssignNew(TableBox, SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(InArgs._Title)
					.TextStyle(&Theme.SubtitleFont)
				]
			]
		];
		// clang-format on

		// Units
		FText Tonnes           = LOCTEXT("Tonnes", "T");
		FText MetersPerSeconds = LOCTEXT("MetersPerSecond", "m/s");
		FText TonnesPerSecond  = LOCTEXT("TonnesPerSecond", "T/s");
		FText Seconds          = LOCTEXT("Seconds", "s");
		FText KiloNewtons      = LOCTEXT("KiloNewtons", "kN");

		// Build the mass table
		AddHeader(LOCTEXT("Overview", "<img src=\"/Text/Module\"/> Overview"));
		AddEntry(LOCTEXT("Name", "Name"), Target.GetName(), Comparison ? Comparison->GetName() : FText());
		AddEntry(LOCTEXT("Classification", "Classification"), Target.GetClassification(),
			Comparison ? Comparison->GetClassification() : FText());
		AddEntry(LOCTEXT("Compartments", "Compartments"), Target.Compartments.Num(), Comparison ? Comparison->Compartments.Num() : -1);

		// Build the mass table
		AddHeader(LOCTEXT("MassMetrics", "<img src=\"/Text/Mass\"/> Mass metrics"));
		AddEntry(LOCTEXT("DryMass", "Dry mass"), TargetPropulsionMetrics.DryMass,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->DryMass : -1, Tonnes);
		AddEntry(LOCTEXT("PropellantMassCapacity", "Propellant capacity"), TargetPropulsionMetrics.PropellantMassCapacity,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->PropellantMassCapacity : -1, Tonnes);
		AddEntry(LOCTEXT("CargoMassCapacity", "Cargo capacity"), TargetPropulsionMetrics.CargoMassCapacity,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->CargoMassCapacity : -1, Tonnes);
		AddEntry(LOCTEXT("MaximumMass", "Full-load mass"), TargetPropulsionMetrics.MaximumMass,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumMass : -1, Tonnes);

		// Build the propulsion table
		AddHeader(LOCTEXT("PropulsionMetrics", "<img src=\"/Text/Thrust\"/> Propulsion metrics"));
		AddEntry(LOCTEXT("SpecificImpulse", "Specific impulse"), TargetPropulsionMetrics.SpecificImpulse,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->SpecificImpulse : -1, Seconds);
		AddEntry(LOCTEXT("Thrust", "Maximum thrust"), TargetPropulsionMetrics.Thrust,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->Thrust : -1, KiloNewtons);
		AddEntry(LOCTEXT("PropellantRate", "Propellant mass rate"), TargetPropulsionMetrics.PropellantRate,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->PropellantRate : -1, TonnesPerSecond);
		AddEntry(LOCTEXT("MaximumDeltaV", "Full-load delta-v"), TargetPropulsionMetrics.MaximumDeltaV,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumDeltaV : -1, MetersPerSeconds);
		AddEntry(LOCTEXT("MaximumBurnTime", "Full-load burn time"), TargetPropulsionMetrics.MaximumBurnTime,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumBurnTime : -1, Seconds);
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
};

#undef LOCTEXT_NAMESPACE
