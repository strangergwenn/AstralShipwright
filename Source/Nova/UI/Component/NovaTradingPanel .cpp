// Nova project - Gwennaël Arbona

#pragma once

#include "NovaTradingPanel.h"

#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"

#include "Nova/UI/Widget/NovaSlider.h"
#include "Nova/UI/Widget/NovaSlider.h"

#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaTradingPanel"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaTradingPanel::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	SNovaModalPanel::Construct(SNovaModalPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
		SAssignNew(InternalWidget, SHorizontalBox)

		+ SHorizontalBox::Slot()

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SNovaButtonLayout)
				.Size("DoubleButtonSize")
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(Theme.VerticalContentPadding)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(this, &SNovaTradingPanel::GetResourceDetails)
						.AutoWrapText(true)
						.WrapTextAt(FNovaStyleSet::GetButtonSize("DoubleButtonSize").Width)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFill)
						[
							SNew(SImage)
							.Image(this, &SNovaTradingPanel::GetResourceImage)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(Theme.VerticalContentPadding)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(this, &SNovaTradingPanel::GetCargoDetails)
						.AutoWrapText(true)
						.WrapTextAt(FNovaStyleSet::GetButtonSize("DoubleButtonSize").Width)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(Theme.VerticalContentPadding)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(FText::FromString("0"))
						]

						+ SHorizontalBox::Slot()

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(this, &SNovaTradingPanel::GetCargoAmount)
						]

						+ SHorizontalBox::Slot()

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(this, &SNovaTradingPanel::GetCargoCapacity)
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaDefaultAssignNew(AmountSlider, SNovaSlider)
				.Size("DoubleButtonSize")
				.ValueStep(10.0f)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(Theme.VerticalContentPadding)
			[
				SNew(SNovaButtonLayout)
				.Size("DoubleButtonSize")
				[
					SNew(SNovaInfoText)
					.Text(this, &SNovaTradingPanel::GetTransactionDetails)
					.Type(this, &SNovaTradingPanel::GetTransactionType)
				]
			]
		]
		
		+ SHorizontalBox::Slot();
	// clang-format on
}

/*----------------------------------------------------
    Interface
----------------------------------------------------*/

void SNovaTradingPanel::StartTrade(
	ANovaSpacecraftPawn* TargetSpacecraftPawn, const UNovaResource* TargetResource, int32 TargetCompartmentIndex)
{
	SpacecraftPawn   = TargetSpacecraftPawn;
	Resource         = TargetResource;
	CompartmentIndex = TargetCompartmentIndex;

	InitialAmount = 0.0f;
	Capacity      = 0.0f;

	if (Resource)
	{
		if (CompartmentIndex != INDEX_NONE)
		{
			InitialAmount = SpacecraftPawn->GetCargoMass(Resource, CompartmentIndex);
			Capacity      = InitialAmount + SpacecraftPawn->GetAvailableCargoMass(Resource, CompartmentIndex);
		}
	}

	AmountSlider->SetMaxValue(Capacity);
	AmountSlider->SetCurrentValue(InitialAmount);

	FSimpleDelegate ConfirmTrade = FSimpleDelegate::CreateSP(this, &SNovaTradingPanel::OnConfirmTrade);
	Show(Resource->Name, FText(), ConfirmTrade, FSimpleDelegate(), FSimpleDelegate());
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

bool SNovaTradingPanel::IsConfirmEnabled() const
{
	return AmountSlider->GetCurrentValue() != InitialAmount;
}

const FSlateBrush* SNovaTradingPanel::GetResourceImage() const
{
	if (Resource)
	{
		return &Resource->AssetRender;
	}

	return nullptr;
}

FText SNovaTradingPanel::GetResourceDetails() const
{
	if (Resource)
	{
		return Resource->Description;
	}

	return FText();
}

FText SNovaTradingPanel::GetCargoAmount() const
{
	return FText::FormatNamed(
		LOCTEXT("CargoAmountFormat", "{amount}T"), TEXT("amount"), FText::AsNumber(FMath::RoundToInt(AmountSlider->GetCurrentValue())));
}

FText SNovaTradingPanel::GetCargoCapacity() const
{
	return FText::FormatNamed(LOCTEXT("CargoCapacityFormat", "{capacity}T"), TEXT("capacity"), FText::AsNumber(Capacity));
}

FText SNovaTradingPanel::GetCargoDetails() const
{
	if (CompartmentIndex == INDEX_NONE)
	{
		return FText::FormatNamed(LOCTEXT("CargoDetailsFormat", "Currently holding {amount}T out of {capacity}T across the spacecraft"),
			TEXT("amount"), FText::AsNumber(InitialAmount), TEXT("capacity"), FText::AsNumber(Capacity));
	}
	else
	{
		return FText::FormatNamed(
			LOCTEXT("CargoDetailsFormat", "Currently holding {amount}T out of {capacity}T in compartment {compartment}"), TEXT("amount"),
			FText::AsNumber(InitialAmount), TEXT("capacity"), FText::AsNumber(Capacity), TEXT("compartment"),
			FText::AsNumber(CompartmentIndex + 1));
	}
}

FText SNovaTradingPanel::GetTransactionDetails() const
{
	return LOCTEXT("TestText", "This transaction will cost you 156 $currency");
}

ENovaInfoBoxType SNovaTradingPanel::GetTransactionType() const
{
	return (AmountSlider->GetCurrentValue() > InitialAmount) ? ENovaInfoBoxType::Positive : ENovaInfoBoxType::Neutral;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaTradingPanel::OnConfirmTrade()
{
	if (Resource)
	{
		SpacecraftPawn->ModifyCargo(Resource, AmountSlider->GetCurrentValue() - InitialAmount, CompartmentIndex);
	}
}

#undef LOCTEXT_NAMESPACE
