// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/NovaUI.h"
#include "Nova/UI/NovaUITypes.h"
#include "Nova/Game/NovaGameTypes.h"
#include "Widgets/SCompoundWidget.h"


class SNovaDeathScreen : public SCompoundWidget
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaDeathScreen)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class UNovaMenuManager>, MenuManager)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	/** Show the death screen */
	void Show(ENovaDamageType Type, FSimpleDelegate Callback);

	/** Hide the death screen */
	void Hide();

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;


	/*----------------------------------------------------
		Content callbacks
	----------------------------------------------------*/

protected:

	FText GetDamageTitleText() const;
	FText GetDamageText() const;


	/*----------------------------------------------------
		Data
	----------------------------------------------------*/

protected:

	// Menu reference
	TWeakObjectPtr<class UNovaMenuManager>        MenuManager;

	// Settings
	float                                         DisplayDuration;

	// State
	bool                                          PlayerWaiting;
	float                                         CurrentDisplayTime;
	ENovaDamageType                               CurrentDamageType;
	FSimpleDelegate                               OnFinished;

};
