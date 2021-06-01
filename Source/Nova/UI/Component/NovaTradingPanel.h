// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaModalPanel.h"
#include "Nova/UI/Widget/NovaSlider.h"

#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaTradingPanel"

/** Heavyweight button class */
class SNovaTradingPanel : public SNovaModalPanel
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTradingPanel)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)

	SLATE_END_ARGS()

public:
	SNovaTradingPanel()
	{}

	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// TODO
		const UNovaResource* TestResource =
			UNovaAssetManager::Get()->GetAsset<UNovaResource>(FGuid("{42C31723-4E30-F22F-1932-EAB2E0E0A3C7}"));
		NCHECK(TestResource);

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
					[
						SNew(STextBlock)
						.TextStyle(&Theme.MainFont)
						.Text(TestResource->Description)
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
							.Image(&TestResource->AssetRender)
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SNovaSlider)
				.Size("DoubleButtonSize")
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SNovaButtonLayout)
				.Size("DoubleButtonSize")
				[
					SNew(SBorder)
					.BorderImage(FNovaStyleSet::GetBrush("Common/SB_White"))
					.BorderBackgroundColor(Theme.PositiveColor)
					.Padding(Theme.ContentPadding)
					[
						SNew(STextBlock)
						.TextStyle(&Theme.InfoFont)
						.Text(LOCTEXT("TestText", "This transaction will cost you 156 $currency"))
					]
				]
			]
		]
		
		+ SHorizontalBox::Slot();
		// clang-format on
	}

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

public:
	/** Start trading */
	void StartTrade()
	{
		// TODO
		const UNovaResource* TestResource =
			UNovaAssetManager::Get()->GetAsset<UNovaResource>(FGuid("{42C31723-4E30-F22F-1932-EAB2E0E0A3C7}"));
		NCHECK(TestResource);

		Show(TestResource->Name, FText(), FSimpleDelegate(), FSimpleDelegate(), FSimpleDelegate());
	}

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
};

#undef LOCTEXT_NAMESPACE
