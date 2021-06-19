// Nova project - Gwennaël Arbona

#pragma once

#include "NovaTradingPanel.h"

#include "Nova/Game/NovaGameTypes.h"
#include "Nova/Player/NovaPlayerController.h"

#include "Nova/Spacecraft/NovaSpacecraft.h"
#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"

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
						SNew(SProgressBar)
						.Style(&Theme.ProgressBarStyle)
						.Percent(this, &SNovaTradingPanel::GetCargoProgress)
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

void SNovaTradingPanel::StartTrade(ANovaPlayerController* TargetPC, const UNovaResource* TargetResource, int32 TargetCompartmentIndex)
{
	PC               = TargetPC;
	Spacecraft       = PC->GetSpacecraft();
	Resource         = TargetResource;
	CompartmentIndex = TargetCompartmentIndex;

	NCHECK(IsValid(PC));
	NCHECK(Spacecraft);

	InitialAmount = 0.0f;
	Capacity      = 0.0f;
	FText Name;

	// Resource mode
	if (Resource)
	{
		Name = Resource->Name;

		if (CompartmentIndex != INDEX_NONE)
		{
			InitialAmount = Spacecraft->GetCargoMass(Resource, CompartmentIndex);
			Capacity      = InitialAmount + Spacecraft->GetAvailableCargoMass(Resource, CompartmentIndex);
		}
	}

	// Fuel mode
	else
	{
		Name = LOCTEXT("FuelName", "Fuel");    // TODO : use a hidden resource for this

		UNovaSpacecraftPropellantSystem* PropellantSystem =
			PC->GetSpacecraftPawn()->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
		NCHECK(PropellantSystem);

		InitialAmount = PropellantSystem->GetCurrentPropellantAmount();
		Capacity      = PropellantSystem->GetTotalPropellantAmount();
	}

	AmountSlider->SetMaxValue(Capacity);
	AmountSlider->SetCurrentValue(InitialAmount);

	FSimpleDelegate ConfirmTrade = FSimpleDelegate::CreateSP(this, &SNovaTradingPanel::OnConfirmTrade);
	Show(Name, FText(), ConfirmTrade, FSimpleDelegate(), FSimpleDelegate());
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
	else
	{    // TODO : use a hidden resource for this
	}

	return nullptr;
}

FText SNovaTradingPanel::GetResourceDetails() const
{
	if (Resource)
	{
		return Resource->Description;
	}
	else
	{
		return LOCTEXT("FuelDescription", "Fuel is cool. Seriously, it's literally cryogenic.");    // TODO : use a hidden resource for this
	}

	return FText();
}

TOptional<float> SNovaTradingPanel::GetCargoProgress() const
{
	return InitialAmount / Capacity;
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
		return FText::FormatNamed(
			LOCTEXT("CargoDetailsFormat", "Currently holding {amount}T out of {capacity}T across the spacecraft ({remaining}T remaining)"),
			TEXT("amount"), FText::AsNumber(InitialAmount), TEXT("capacity"), FText::AsNumber(Capacity), TEXT("remaining"),
			FText::AsNumber(Capacity - InitialAmount));
	}
	else
	{
		return FText::FormatNamed(
			LOCTEXT("CargoDetailsFormat",
				"Currently holding {amount}T out of {capacity}T in compartment {compartment} ({remaining}T remaining)"),
			TEXT("amount"), FText::AsNumber(InitialAmount), TEXT("capacity"), FText::AsNumber(Capacity), TEXT("compartment"),
			FText::AsNumber(CompartmentIndex + 1), TEXT("remaining"), FText::AsNumber(Capacity - InitialAmount));
	}
}

FText SNovaTradingPanel::GetTransactionDetails() const
{
	// TODO : cost system
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
	// Resource mode
	if (Resource)
	{
		FNovaSpacecraft ModifiedSpacecraft = Spacecraft->GetSafeCopy();
		ModifiedSpacecraft.ModifyCargo(Resource, AmountSlider->GetCurrentValue() - InitialAmount, CompartmentIndex);
		PC->UpdateSpacecraft(ModifiedSpacecraft);
	}

	// Fuel mode
	else
	{
		UNovaSpacecraftPropellantSystem* PropellantSystem =
			PC->GetSpacecraftPawn()->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
		NCHECK(PropellantSystem);

		PropellantSystem->SetPropellantAmount(AmountSlider->GetCurrentValue());
	}
}

#undef LOCTEXT_NAMESPACE
