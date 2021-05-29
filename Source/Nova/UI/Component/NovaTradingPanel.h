// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/Widget/NovaButton.h"

/** Heavyweight button class */
class SNovaTradingPanel : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate args
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaTradingPanel)
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)
	SLATE_ARGUMENT(class SNovaNavigationPanel*, Panel)

	SLATE_END_ARGS()

public:
	SNovaTradingPanel()
	{}

	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

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
