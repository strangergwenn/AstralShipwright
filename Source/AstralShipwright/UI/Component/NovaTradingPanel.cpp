// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "NovaTradingPanel.h"

#include "Game/NovaGameTypes.h"
#include "Game/NovaGameState.h"
#include "System/NovaGameInstance.h"
#include "Player/NovaPlayerController.h"

#include "Spacecraft/NovaSpacecraft.h"
#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "UI/Component/NovaTradableAssetItem.h"
#include "UI/Widget/NovaSlider.h"

#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaTradingPanel"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaTradingPanel::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	SNovaModalPanel::Construct(SNovaModalPanel::FArguments().Menu(InArgs._Menu));
	uint32 TextWidth = FNovaStyleSet::GetButtonSize("DoubleButtonSize").Width - Theme.ContentPadding.Left - Theme.ContentPadding.Right;

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

				// Asset render
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				[
					SAssignNew(ResourceItem, SNovaTradableAssetItem)
					.Dark(true)
				]

				// Text box
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBackground)
					.Padding(Theme.ContentPadding)
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
							.WrapTextAt(TextWidth)
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
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							.Text(this, &SNovaTradingPanel::GetCargoDetails)
							.AutoWrapText(true)
							.WrapTextAt(TextWidth)
						]
					]
				]

				// Caption box
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Theme.VerticalContentPadding)
				[
					SAssignNew(CaptionBox, SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(this, &SNovaTradingPanel::GetCargoMinValue)
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
						.Text(this, &SNovaTradingPanel::GetCargoMaxValue)
					]
				]
			]
		]

		// Slider
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
				SAssignNew(InfoText, SNovaInfoText)
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

void SNovaTradingPanel::ShowPanelInternal(ANovaPlayerController* TargetPC, const UNovaArea* TargetArea, const UNovaResource* TargetResource,
	int32 TargetCompartmentIndex, bool AllowTrade)
{
	PC               = TargetPC;
	Spacecraft       = PC->GetSpacecraft();
	Area             = TargetArea;
	Resource         = TargetResource;
	CompartmentIndex = TargetCompartmentIndex;
	IsTradeAllowed   = AllowTrade;

	const ANovaGameState* GameState = PC->GetWorld()->GetGameState<ANovaGameState>();
	NCHECK(IsValid(GameState));

	NCHECK(PC.IsValid());
	NCHECK(IsValid(Resource));
	NCHECK(Spacecraft);

	InitialAmount = 0.0f;
	MinAmount     = 0.0f;
	MaxAmount     = 0.0f;
	Capacity      = 0.0f;

	// Resource mode
	if (Resource != UNovaResource::GetPropellant())
	{
		if (CompartmentIndex != INDEX_NONE)
		{
			InitialAmount = Spacecraft->GetCargoMass(Resource, CompartmentIndex);
			Capacity      = InitialAmount + Spacecraft->GetAvailableCargoMass(Resource, CompartmentIndex);

			if (GameState->IsResourceSold(Resource))
			{
				MinAmount = InitialAmount;
				MaxAmount = Capacity;
			}
			else
			{
				MinAmount = 0.0f;
				MaxAmount = InitialAmount;
			}
		}
	}

	// Propellant mode
	else
	{
		UNovaSpacecraftPropellantSystem* PropellantSystem = GameState->GetSpacecraftSystem<UNovaSpacecraftPropellantSystem>(Spacecraft);
		NCHECK(PropellantSystem);

		InitialAmount = PropellantSystem->GetCurrentPropellantMass();
		Capacity      = PropellantSystem->GetPropellantCapacity();
		MaxAmount     = Capacity;
	}
	ResourceItem->SetArea(TargetArea);
	ResourceItem->SetAsset(TargetResource);
	ResourceItem->SetGameState(GameState);

	// Setup the UI
	if (AllowTrade)
	{
		InfoText->SetVisibility(EVisibility::Visible);
		CaptionBox->SetVisibility(EVisibility::Visible);
		AmountSlider->SetVisibility(EVisibility::Visible);
		AmountSlider->SetMinValue(MinAmount);
		AmountSlider->SetMaxValue(MaxAmount);
		AmountSlider->SetCurrentValue(InitialAmount);

		FSimpleDelegate ConfirmTrade = FSimpleDelegate::CreateSP(this, &SNovaTradingPanel::OnConfirmTrade);

		// Build text details
		FText TradeTitle;
		if (Resource == UNovaResource::GetPropellant())
		{
			TradeTitle =
				FText::FormatNamed(LOCTEXT("PropellantTitleFormat", "Trade propellant with {station}"), TEXT("station"), Area->Name);
		}
		else if (GameState->IsResourceSold(Resource))
		{
			TradeTitle = FText::FormatNamed(LOCTEXT("BuyTitleFormat", "Buy resource from {station}"), TEXT("station"), Area->Name);
		}
		else if (InitialAmount != 0 && !GameState->IsResourceSold(Resource))
		{
			TradeTitle = FText::FormatNamed(LOCTEXT("SellTitleFormat", "Sell resource to {station}"), TEXT("station"), Area->Name);
		}

		Show(TradeTitle, FText(), ConfirmTrade);
	}
	else
	{
		InfoText->SetVisibility(EVisibility::Collapsed);
		CaptionBox->SetVisibility(EVisibility::Collapsed);
		AmountSlider->SetVisibility(EVisibility::Collapsed);

		Show(LOCTEXT("InspectTitle", "Inspect cargo slot"), FText(), FSimpleDelegate());
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

bool SNovaTradingPanel::IsConfirmEnabled() const
{
	return IsTradeAllowed && AmountSlider->GetCurrentValue() != InitialAmount && PC.IsValid() &&
		   PC->CanAffordTransaction(GetTransactionValue());
}

FText SNovaTradingPanel::GetResourceDetails() const
{
	if (Resource)
	{
		return Resource->Description;
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
		LOCTEXT("CargoAmountFormat", "{amount} T"), TEXT("amount"), FText::AsNumber(FMath::RoundToInt(AmountSlider->GetCurrentValue())));
}

FText SNovaTradingPanel::GetCargoMinValue() const
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 1;

	return FText::FormatNamed(LOCTEXT("CargoMinFormat", "{min} T (min)"), TEXT("min"), FText::AsNumber(MinAmount, &Options));
}

FText SNovaTradingPanel::GetCargoMaxValue() const
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 1;

	return FText::FormatNamed(LOCTEXT("CargoMaxFormat", "{max} T (max)"), TEXT("max"), FText::AsNumber(MaxAmount, &Options));
}

FText SNovaTradingPanel::GetCargoDetails() const
{
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 1;

	if (CompartmentIndex == INDEX_NONE)
	{
		return FText::FormatNamed(LOCTEXT("CargoDetailsFormat",
									  "Currently holding {amount} T out of {capacity} T across the spacecraft ({remaining} T remaining)"),
			TEXT("amount"), FText::AsNumber(InitialAmount, &Options), TEXT("capacity"), FText::AsNumber(Capacity, &Options),
			TEXT("remaining"), FText::AsNumber(Capacity - InitialAmount, &Options));
	}
	else
	{
		return FText::FormatNamed(
			LOCTEXT("CargoDetailsFormat",
				"Currently holding {amount} T out of {capacity} T in compartment {compartment} ({remaining} T remaining)"),
			TEXT("amount"), FText::AsNumber(InitialAmount, &Options), TEXT("capacity"), FText::AsNumber(Capacity, &Options),
			TEXT("compartment"), FText::AsNumber(CompartmentIndex + 1, &Options), TEXT("remaining"),
			FText::AsNumber(Capacity - InitialAmount, &Options));
	}
}

FText SNovaTradingPanel::GetTransactionDetails() const
{
	FNovaCredits CreditsValue = GetTransactionValue();

	if (CreditsValue <= 0)
	{
		return FText::FormatNamed(
			LOCTEXT("TransactionCostDetails", "This transaction will cost you {amount}"), TEXT("amount"), GetPriceText(-CreditsValue));
	}
	else
	{
		return FText::FormatNamed(
			LOCTEXT("TransactionGainDetails", "This transaction will gain you {amount}"), TEXT("amount"), GetPriceText(CreditsValue));
	}
}

ENovaInfoBoxType SNovaTradingPanel::GetTransactionType() const
{
	FNovaCredits CreditsValue = GetTransactionValue();

	if (PC.IsValid() && !PC->CanAffordTransaction(GetTransactionValue()))
	{
		return ENovaInfoBoxType::Negative;
	}
	else if (CreditsValue == 0)
	{
		return ENovaInfoBoxType::Neutral;
	}
	else
	{
		return ENovaInfoBoxType::Positive;
	}
}

FNovaCredits SNovaTradingPanel::GetTransactionValue() const
{
	if (PC.IsValid())
	{
		const ANovaGameState* GameState = PC->GetWorld()->GetGameState<ANovaGameState>();

		if (Resource && IsValid(GameState))
		{
			float        Amount    = AmountSlider->GetCurrentValue() - InitialAmount;
			FNovaCredits UnitPrice = GameState->GetCurrentPrice(Resource, Area, Amount < 0);
			return -Amount * UnitPrice;
		}
	}

	return 0;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaTradingPanel::OnConfirmTrade()
{
	FNovaSpacecraft ModifiedSpacecraft = Spacecraft->GetSafeCopy();

	// Resource mode
	if (Resource != UNovaResource::GetPropellant())
	{
		ModifiedSpacecraft.ModifyCargo(Resource, AmountSlider->GetCurrentValue() - InitialAmount, CompartmentIndex);
	}

	// Propellant mode
	else
	{
		ModifiedSpacecraft.SetPropellantMass(AmountSlider->GetCurrentValue());
	}

	// Process spacecraft update and payment
	NCHECK(PC.IsValid());
	PC->UpdateSpacecraft(ModifiedSpacecraft);
	PC->ProcessTransaction(GetTransactionValue());
	PC->GetGameInstance<UNovaGameInstance>()->SaveGame(PC.Get());
}

#undef LOCTEXT_NAMESPACE
