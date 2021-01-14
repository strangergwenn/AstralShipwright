// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/Nova.h"
#include "Nova/UI/NovaUI.h"

#include "Widgets/SCompoundWidget.h"


/** Key label to show which keys are bound to an action */
class SNovaKeyLabel : public SCompoundWidget
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaKeyLabel)
		: _Alpha(1)
	{}
	
	SLATE_ATTRIBUTE(FKey, Key)
	SLATE_ATTRIBUTE(float, Alpha)

	SLATE_END_ARGS()


public:

	/*----------------------------------------------------
		Constructor
	----------------------------------------------------*/

	SNovaKeyLabel()
		: SCompoundWidget()
	{}

	void Construct(const FArguments& InArgs);


	/*----------------------------------------------------
		Callbacks
	----------------------------------------------------*/

protected:

	FText GetKeyText() const;

	const FSlateBrush* GetKeyIcon() const;

	FSlateColor GetKeyIconColor() const;

	FSlateColor GetKeyTextColor() const;


	/*----------------------------------------------------
		Private data
	----------------------------------------------------*/

protected:

	// Attributes
	TAttribute<FKey>                              KeyName;
	TAttribute<float>                             CurrentAlpha;

};
