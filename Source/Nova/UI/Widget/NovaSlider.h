// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/Nova.h"
#include "Nova/UI/Widget/NovaButton.h"

/** Slider control class */
class SNovaSlider : public SNovaButton
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaSlider)
		: _Theme("DefaultButton")
		, _Size("DefaultButtonSize")
		, _Enabled(true)
		, _Analog(false)
		, _Value(0)
		, _MinValue(0)
		, _MaxValue(1)
		, _ValueStep(1)
	{}

	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)

	SLATE_NAMED_SLOT(FArguments, Header)
	SLATE_NAMED_SLOT(FArguments, Footer)

	SLATE_ATTRIBUTE(bool, Enabled)
	SLATE_ARGUMENT(bool, Analog)
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

	bool HorizontalAnalogInput(float Value) override;

	/** Get the current value */
	float GetCurrentValue() const;

	/** Get the minimum value */
	float GetMinValue() const;

	/** Get the maximum value */
	float GetMaxValue() const;

	/** Set the maximum value */
	void SetMaxValue(float Value);

	/** Set the current value */
	void SetCurrentValue(float Value);

	/*----------------------------------------------------
	    Callbacks
	----------------------------------------------------*/

protected:
	/** Color callback*/
	FLinearColor GetColor() const;

	/** Color callback*/
	FSlateColor GetSlateColor() const;

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
	float SliderSpeed;
	float SliderAnalogSpeed;
	bool  Analog;

	// State
	float                ValueStep;
	float                CurrentValue;
	bool                 IsMouseCaptured;
	FOnFloatValueChanged ValueChanged;

	// Slate elements
	TSharedPtr<SSlider> Slider;
};
