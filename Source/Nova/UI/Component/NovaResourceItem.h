// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/Game/NovaGameTypes.h"
#include "Widgets/Layout/SScaleBox.h"

/** Nova resource item image */
class SNovaResourceItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaResourceItem) : _Resource(nullptr)
	{}

	SLATE_ARGUMENT(const UNovaResource*, Resource)

	SLATE_END_ARGS()

public:
	SNovaResourceItem() : Resource(nullptr)
	{}

	void Construct(const FArguments& InArgs)
	{
		Resource = InArgs._Resource;

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
						SNew(SImage).Image(this, &SNovaResourceItem::GetBrush)
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
						.BorderImage(&Theme.MainMenuDarkBackground)
						.Padding(Theme.ContentPadding)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.MainFont)
							 .Text(this, &SNovaResourceItem::GetName)
						]
					 ]

					+ SHorizontalBox::Slot()

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(Theme.ContentPadding)
					[
						SNew(SRichTextBlock)
						.Text(this, &SNovaResourceItem::GetPrice)
						.TextStyle(&Theme.MainFont)
						.DecoratorStyleSet(&FNovaStyleSet::GetStyle())
						+ SRichTextBlock::ImageDecorator()
					]
				]
			]
		];
		// clang-format on
	}

	void SetResource(const UNovaResource* NewResource)
	{
		Resource = NewResource;
	}

protected:
	const FSlateBrush* GetBrush() const
	{
		if (Resource)
		{
			return &Resource->AssetRender;
		}

		return nullptr;
	}

	FText GetName() const
	{
		if (Resource)
		{
			return Resource->Name;
		}

		return FText();
	}

	FText GetPrice() const
	{
		if (Resource)
		{
			return GetPriceText(Resource->BasePrice);
		}

		return FText();
	}

protected:
	// General data
	const UNovaResource* Resource;
};
