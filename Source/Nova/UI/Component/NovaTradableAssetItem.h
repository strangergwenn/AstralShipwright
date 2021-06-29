// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/Game/NovaGameTypes.h"
#include "Widgets/Layout/SScaleBox.h"

/** Nova resource item image */
class SNovaTradableAssetItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaTradableAssetItem) : _Asset(nullptr), _GameState(nullptr), _ForSale(false), _Dark(false)
	{}

	SLATE_ARGUMENT(const UNovaTradableAssetDescription*, Asset)
	SLATE_ARGUMENT(const ANovaGameState*, GameState)
	SLATE_ARGUMENT(bool, ForSale)
	SLATE_ARGUMENT(bool, Dark)

	SLATE_END_ARGS()

public:
	SNovaTradableAssetItem() : Asset(nullptr)
	{}

	void Construct(const FArguments& InArgs)
	{
		Asset     = InArgs._Asset;
		GameState = InArgs._GameState;
		ForSale   = InArgs._ForSale;

		const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
		const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

		// clang-format off
		ChildSlot
		[
			SNew(SBox)
			.HeightOverride(FNovaStyleSet::GetButtonSize("LargeListButtonSize").Height)
			[
				SNew(SOverlay)
				.Clipping(EWidgetClipping::ClipToBoundsAlways)

				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					[
						SNew(SImage).Image(this, &SNovaTradableAssetItem::GetBrush)
					]
				]

				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					.HAlign(HAlign_Right)
					[
						SNew(SImage)
						.Image(FNovaStyleSet::GetBrush("Common/SB_Corner"))
						.ColorAndOpacity(Theme.PositiveColor)
					]
				]

				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.BorderImage(InArgs._Dark ? &Theme.MainMenuDarkBackground : new FSlateNoResource)
						.Padding(Theme.ContentPadding)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(this, &SNovaTradableAssetItem::GetName)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SRichTextBlock)
								.TextStyle(&Theme.MainFont)
								.Text(this, &SNovaTradableAssetItem::GetDescription)
								.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
								+ SRichTextBlock::ImageDecorator()
							]
						]
					 ]

					+ SHorizontalBox::Slot()

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(Theme.ContentPadding)
					[
						SNew(SRichTextBlock)
						.Text(this, &SNovaTradableAssetItem::GetPrice)
						.TextStyle(&Theme.MainFont)
						.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
						+ SRichTextBlock::ImageDecorator()
					]
				]
			]
		];
		// clang-format on
	}

	void SetAsset(const UNovaTradableAssetDescription* NewAsset)
	{
		Asset = NewAsset;
	}

	void SetGameState(const ANovaGameState* NewGameState)
	{
		GameState = NewGameState;
	}

protected:
	const FSlateBrush* GetBrush() const
	{
		if (Asset.IsValid())
		{
			return &Asset->AssetRender;
		}

		return nullptr;
	}

	FText GetName() const
	{
		if (Asset.IsValid())
		{
			return Asset->Name;
		}

		return FText();
	}

	FText GetDescription() const
	{
		if (Asset.IsValid())
		{
			return Asset->GetParagraphDescription();
		}

		return FText();
	}

	FText GetPrice() const
	{
		if (Asset.IsValid() && GameState.IsValid())
		{
			double Price = GameState->GetCurrentPrice(Asset.Get(), ForSale);
			return GetPriceText(Price);
		}

		return FText();
	}

protected:
	// Settings
	TWeakObjectPtr<const UNovaTradableAssetDescription> Asset;
	TWeakObjectPtr<const ANovaGameState>                GameState;
	bool                                                ForSale;
};
