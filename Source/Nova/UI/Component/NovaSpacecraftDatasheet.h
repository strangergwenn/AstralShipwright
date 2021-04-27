// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"

/** Spacecraft datasheet class */
class SNovaSpacecraftDatasheet : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaSpacecraftDatasheet) : _TargetSpacecraft(), _ComparisonSpacecraft(nullptr)
	{}

	SLATE_ARGUMENT(FNovaSpacecraft, TargetSpacecraft)
	SLATE_ARGUMENT(const FNovaSpacecraft*, ComparisonSpacecraft)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// clang-format off
		ChildSlot
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Placeholder")))
			.TextStyle(&Theme.MainFont)
		];
		// clang-format on
	}
};
