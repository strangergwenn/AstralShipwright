// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/Game/NovaGameTypes.h"
#include "Widgets/Layout/SScaleBox.h"

/** Nova resource item image */
class SNovaTradableAssetItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaTradableAssetItem)
		: _Asset(nullptr), _GameState(nullptr), _NoPriceHint(false), _Dark(false), _SelectionIcon(nullptr)
	{}

	SLATE_ARGUMENT(const UNovaTradableAssetDescription*, Asset)
	SLATE_ARGUMENT(const ANovaGameState*, GameState)
	SLATE_ARGUMENT(bool, NoPriceHint)
	SLATE_ARGUMENT(bool, Dark)
	SLATE_ATTRIBUTE(const FSlateBrush*, SelectionIcon)

	SLATE_END_ARGS()

public:
	SNovaTradableAssetItem() : Asset(nullptr)
	{}

	void Construct(const FArguments& InArgs)
	{
		Asset     = InArgs._Asset;
		GameState = InArgs._GameState;

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
					.HAlign(HAlign_Right)
					[
						SNew(SImage)
						.Image(FNovaStyleSet::GetBrush("Common/SB_Corner"))
						.ColorAndOpacity(Theme.PositiveColor)
					]
				]

				+ SOverlay::Slot()
				[
					SAssignNew(ContentBox, SHorizontalBox)

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

	FText GetPriceText() const
	{
		if (Asset.IsValid() && GameState.IsValid())
		{
			FNovaCredits Price = GameState->GetCurrentPrice(Asset.Get());
			return ::GetPriceText(Price);
		}

		return FText();
	}

	const FSlateBrush* GetMarketHintIcon() const
	{
		if (Asset.IsValid() && GameState.IsValid())
		{
			switch (GameState->GetCurrentPriceModifier(Asset.Get()))
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
	TWeakObjectPtr<const UNovaTradableAssetDescription> Asset;
	TWeakObjectPtr<const ANovaGameState>                GameState;
};
