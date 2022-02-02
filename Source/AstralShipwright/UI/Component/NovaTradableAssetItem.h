// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Game/NovaArea.h"
#include "Game/NovaGameTypes.h"
#include "Widgets/Layout/SScaleBox.h"

/** Nova resource item image */
class SNovaTradableAssetItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaTradableAssetItem)
		: _Area(nullptr)
		, _Asset(nullptr)
		, _DefaultAsset(nullptr)
		, _GameState(nullptr)
		, _NoPriceHint(false)
		, _Dark(false)
		, _SelectionIcon(nullptr)
	{}

	SLATE_ARGUMENT(const UNovaArea*, Area)
	SLATE_ARGUMENT(const UNovaTradableAssetDescription*, Asset)
	SLATE_ARGUMENT(const UNovaTradableAssetDescription*, DefaultAsset)
	SLATE_ARGUMENT(const ANovaGameState*, GameState)
	SLATE_ARGUMENT(bool, NoPriceHint)
	SLATE_ARGUMENT(bool, Dark)
	SLATE_ATTRIBUTE(const FSlateBrush*, SelectionIcon)

	SLATE_END_ARGS()

public:
	SNovaTradableAssetItem() : Area(nullptr), Asset(nullptr), DefaultAsset(nullptr), GameState(nullptr)
	{}

	void Construct(const FArguments& InArgs)
	{
		Area         = InArgs._Area;
		Asset        = InArgs._Asset;
		DefaultAsset = InArgs._DefaultAsset;
		GameState    = InArgs._GameState;

		if (!Area.IsValid() && GameState.IsValid())
		{
			Area = GameState->GetCurrentArea();
		}

		const FNovaMainTheme&      Theme       = FNovaStyleSet::GetMainTheme();
		const FNovaButtonTheme&    ButtonTheme = FNovaStyleSet::GetButtonTheme();
		TSharedPtr<SHorizontalBox> ContentBox;

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
					.HAlign(HAlign_Left)
					[
						SNew(SImage)
						.Image(InArgs._Dark ? FNovaStyleSet::GetBrush("Common/SB_Corner_Left") : new FSlateNoResource)
					]
				]

				+ SOverlay::Slot()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					.HAlign(HAlign_Right)
					[
						SNew(SImage)
						.Image(FNovaStyleSet::GetBrush("Common/SB_Corner_Right"))
						.ColorAndOpacity(Theme.PositiveColor)
					]
				]

				+ SOverlay::Slot()
				[
					SAssignNew(ContentBox, SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
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

					+ SHorizontalBox::Slot()

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(Theme.ContentPadding)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SRichTextBlock)
							.Text(this, &SNovaTradableAssetItem::GetPriceText)
							.TextStyle(&Theme.MainFont)
							.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
							+ SRichTextBlock::ImageDecorator()
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Right)
						[
							SNew(SImage)
							.Image(this, &SNovaTradableAssetItem::GetMarketHintIcon)
							.Visibility(InArgs._NoPriceHint ? EVisibility::Collapsed : EVisibility::Visible)
						]
					]
				]
			]
		];

		// Add the selection icon
		if (InArgs._SelectionIcon.IsSet() || InArgs._SelectionIcon.IsBound())
		{
			ContentBox->InsertSlot(0)
			.AutoWidth()
			.VAlign(VAlign_Top)
			[
				SNew(SImage)
				.Image_Lambda([=]()
				{
					return InArgs._SelectionIcon.Get();
				})
			];
		}

		// clang-format on
	}

	void SetArea(const UNovaArea* NewArea)
	{
		Area = NewArea;
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
		else if (DefaultAsset.IsValid())
		{
			return &DefaultAsset->AssetRender;
		}

		return nullptr;
	}

	FText GetName() const
	{
		if (Asset.IsValid())
		{
			return Asset->Name;
		}
		else if (DefaultAsset.IsValid())
		{
			return DefaultAsset->Name;
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

	FText GetPriceText() const
	{
		if (GameState.IsValid())
		{
			if (Asset.IsValid())
			{
				FNovaCredits Price = GameState->GetCurrentPrice(Asset.Get(), Area.Get(), false);
				return ::GetPriceText(Price);
			}
			else if (DefaultAsset.IsValid())
			{
				FNovaCredits Price = GameState->GetCurrentPrice(DefaultAsset.Get(), Area.Get(), false);
				return ::GetPriceText(Price);
			}
		}

		return FText();
	}

	const FSlateBrush* GetMarketHintIcon() const
	{
		if (Asset.IsValid() && GameState.IsValid())
		{
			switch (GameState->GetCurrentPriceModifier(Asset.Get(), Area.Get()))
			{
				case ENovaPriceModifier::Cheap:
					return FNovaStyleSet::GetBrush("Icon/SB_BelowAverage");
				case ENovaPriceModifier::BelowAverage:
					return FNovaStyleSet::GetBrush("Icon/SB_BelowAverage");
				case ENovaPriceModifier::Average:
					return FNovaStyleSet::GetBrush("Icon/SB_Average");
				case ENovaPriceModifier::AboveAverage:
					return FNovaStyleSet::GetBrush("Icon/SB_AboveAverage");
				case ENovaPriceModifier::Expensive:
					return FNovaStyleSet::GetBrush("Icon/SB_Expensive");
			}
		}

		return nullptr;
	}

protected:
	// Settings
	TWeakObjectPtr<const UNovaArea>                     Area;
	TWeakObjectPtr<const UNovaTradableAssetDescription> Asset;
	TWeakObjectPtr<const UNovaTradableAssetDescription> DefaultAsset;
	TWeakObjectPtr<const ANovaGameState>                GameState;
};
