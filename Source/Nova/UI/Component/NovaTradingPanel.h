// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaModalPanel.h"

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

		SNovaModalPanel::Construct(SNovaModalPanel::FArguments().Menu(InArgs._Menu));

		// clang-format off
		// clang-format on
	}

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

public:
	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

protected:
};
