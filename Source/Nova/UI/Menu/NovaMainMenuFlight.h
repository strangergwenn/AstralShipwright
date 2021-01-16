// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/Widget/NovaTabView.h"
#include "Nova/UI/Widget/NovaListView.h"

#include "Online.h"


class SNovaMainMenuFlight : public SNovaTabPanel
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaMainMenuFlight)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)
	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	virtual void Show() override;

	virtual void Hide() override;

	virtual void HorizontalAnalogInput(float Value) override;

	virtual void VerticalAnalogInput(float Value) override;


	/*----------------------------------------------------
		Internals
	----------------------------------------------------*/

protected:

	/** Get the factory presentation pawn */
	class ANovaSpacecraftAssembly* GetSpacecraftAssembly() const;


	/*----------------------------------------------------
		Content callbacks
	----------------------------------------------------*/

protected:


	/*----------------------------------------------------
		Callbacks
	----------------------------------------------------*/



	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Menu manager
	TWeakObjectPtr<UNovaMenuManager>              MenuManager;

};
