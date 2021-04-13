// Nova project - Gwennaël Arbona

#include "NovaKeyLabel.h"
#include "Nova/UI/NovaUI.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaKeyLabel::Construct(const FArguments& InArgs)
{
	// Setup
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	KeyName                     = InArgs._Key;
	CurrentAlpha                = InArgs._Alpha;

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(this, &SNovaKeyLabel::GetKeyIcon)
			.ColorAndOpacity(this, &SNovaKeyLabel::GetKeyIconColor)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.KeyFont)
			.Text(this, &SNovaKeyLabel::GetKeyText)
			.ColorAndOpacity(this, &SNovaKeyLabel::GetKeyTextColor)
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FText SNovaKeyLabel::GetKeyText() const
{
	return FNovaStyleSet::GetKeyDisplay(KeyName.Get()).Key;
}

const FSlateBrush* SNovaKeyLabel::GetKeyIcon() const
{
	return FNovaStyleSet::GetKeyDisplay(KeyName.Get()).Value;
}

FSlateColor SNovaKeyLabel::GetKeyIconColor() const
{
	return FLinearColor(1, 1, 1, CurrentAlpha.Get());
}

FSlateColor SNovaKeyLabel::GetKeyTextColor() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	FLinearColor Color = Theme.KeyFont.ColorAndOpacity.GetSpecifiedColor();
	Color.A *= CurrentAlpha.Get();

	return Color;
}
