// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/Nova.h"
#include "Nova/UI/NovaUI.h"

#include "Widgets/SCompoundWidget.h"


/** Slider control class */
class SNovaSlider : public SCompoundWidget
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaSlider)
		: _Theme("DefaultSlider")
		, _ControlsTheme("DefaultButton")
		, _ControlsSize("SmallButtonSize")
		, _Enabled(true)
		, _Value(0)
		, _MinValue(0)
		, _MaxValue(1)
		, _ValueStep(1)
	{}

	SLATE_ARGUMENT(class SNovaNavigationPanel*, Panel)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, ControlsTheme)
	SLATE_ARGUMENT(FName, ControlsSize)
		
	SLATE_ATTRIBUTE(bool, Enabled)
	SLATE_ARGUMENT(float, Value)
	SLATE_ARGUMENT(float, MinValue)
	SLATE_ARGUMENT(float, MaxValue)
	SLATE_ARGUMENT(float, ValueStep)

	SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)
	
	SLATE_END_ARGS()


public:

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
		Public methods
	----------------------------------------------------*/

	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	/** Get the current value */
	float GetCurrentValue() const;

	/** Set the current value */
	void SetCurrentValue(float Value);


	/*----------------------------------------------------
		Callbacks
	----------------------------------------------------*/

protected:

	/** Brush callback*/
	const FSlateBrush* GetBackgroundBrush() const;

	/** Value callback */
	void OnSliderValueChanged(float Value);

	/** Mouse was captured and user is changing the value */
	void OnMouseCaptured();

	/** Mouse was released and user is no longer changing the value */
	void OnMouseReleased();

	/** Add ValueStep to the current value */
	void OnIncrement();

	/** Subtract ValueStep from the current value */
	void OnDecrement();


protected:

	/*----------------------------------------------------
		Private data
	----------------------------------------------------*/
	
	// Settings & attributes
	FName                                         ThemeName;
	float                                         SliderSpeed;

	// State
	float                                         ValueStep;
	float                                         CurrentValue;
	bool                                          IsMouseCaptured;
	FOnFloatValueChanged                          ValueChanged;
	
	// Slate elements
	TSharedPtr<SSlider>                           Slider;

};
