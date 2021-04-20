// Nova project - Gwennaël Arbona

#include "NovaSlider.h"
#include "NovaButton.h"
#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaSlider::Construct(const FArguments& InArgs)
{
	// Arguments
	SliderThemeName = InArgs._SliderTheme;
	EnabledState    = InArgs._Enabled;
	ValueStep       = InArgs._ValueStep;
	Analog          = InArgs._Analog;
	CurrentValue    = InArgs._Value;
	ValueChanged    = InArgs._OnValueChanged;

	// Setup
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();
	const FNovaSliderTheme& Theme       = FNovaStyleSet::GetTheme<FNovaSliderTheme>(SliderThemeName);
	const FNovaButtonSize&  Size        = FNovaStyleSet::GetButtonSize(InArgs._Size);
	SliderSpeed                         = 2.0f;
	SliderAnalogSpeed                   = 0.01f;

	// Border padding
	FMargin HeaderBorderPadding = FMargin(Theme.SliderStyle.NormalThumbImage.GetImageSize().X + ButtonTheme.AnimationPadding.Left, 0);

	// Parent constructor
	SNovaButton::Construct(SNovaButton::FArguments()
							   .HelpText(InArgs._HelpText)
							   .Size(InArgs._Size)
							   .Theme(InArgs._Theme)
							   .Enabled(InArgs._Enabled)
							   .Header()[SNew(SBox).Padding(HeaderBorderPadding)[InArgs._Header.Widget]]
							   .Footer()[SNew(SBox).Padding(HeaderBorderPadding)[InArgs._Footer.Widget]]);

	// Internal content
	InnerContainer->SetContent(SAssignNew(Slider, SSlider)
								   .IsFocusable(false)
								   .Value(InArgs._Value)
								   .MinValue(InArgs._MinValue)
								   .MaxValue(InArgs._MaxValue)
								   .OnValueChanged(this, &SNovaSlider::OnSliderValueChanged)
								   .OnMouseCaptureBegin(this, &SNovaSlider::OnMouseCaptured)
								   .OnMouseCaptureEnd(this, &SNovaSlider::OnMouseReleased)
								   .Style(&Theme.SliderStyle)
								   .SliderBarColor(FLinearColor(0, 0, 0, 0)));
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaSlider::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaButton::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	if (Slider->GetValue() != CurrentValue)
	{
		float Delta = CurrentValue - Slider->GetValue();
		float Range = Slider->GetMaxValue() - Slider->GetMinValue();
		Delta       = FMath::Clamp(Delta, -SliderSpeed * Range * DeltaTime, SliderSpeed * Range * DeltaTime);

		Slider->SetValue(Slider->GetValue() + Delta);
	}
}

bool SNovaSlider::HorizontalAnalogInput(float Value)
{
	float Range = Slider->GetMaxValue() - Slider->GetMinValue();

	if (Analog)
	{
		OnSliderValueChanged(CurrentValue + SliderAnalogSpeed * Range * Value);
	}
	else if (Slider->GetValue() == CurrentValue)
	{
		if (Value >= 1.0f)
		{
			OnIncrement();
		}
		else if (Value <= -1.0f)
		{
			OnDecrement();
		}
	}

	return true;
}

float SNovaSlider::GetCurrentValue() const
{
	return CurrentValue;
}

float SNovaSlider::GetMinValue() const
{
	return Slider->GetMinValue();
}

float SNovaSlider::GetMaxValue() const
{
	return Slider->GetMaxValue();
}

void SNovaSlider::SetCurrentValue(float Value)
{
	CurrentValue = Value;
	Slider->SetValue(CurrentValue);
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FLinearColor SNovaSlider::GetColor() const
{
	const UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);

	return MenuManager->GetInterfaceColor();
}

FSlateColor SNovaSlider::GetSlateColor() const
{
	return GetColor();
}

const FSlateBrush* SNovaSlider::GetBackgroundBrush() const
{
	const FNovaSliderTheme& Theme = FNovaStyleSet::GetTheme<FNovaSliderTheme>(SliderThemeName);

	if (!IsEnabled())
	{
		return &Theme.SliderStyle.DisabledBarImage;
	}
	else
	{
		return &Theme.SliderStyle.NormalBarImage;
	}
}

void SNovaSlider::OnSliderValueChanged(float Value)
{
	Value = FMath::Clamp(Value, Slider->GetMinValue(), Slider->GetMaxValue());
	if (Value != CurrentValue)
	{
		if (Analog)
		{
			ValueChanged.ExecuteIfBound(Value);
		}
		else
		{
			float StepCount = 1.0f / ValueStep;
			float StepValue = FMath::RoundToInt(StepCount * Value);
			Value           = StepValue / StepCount;

			if (Value != CurrentValue)
			{
				ValueChanged.ExecuteIfBound(Value);
			}

			if (IsMouseCaptured)
			{
				Slider->SetValue(Value);
			}
		}

		CurrentValue = Value;
	}
}

void SNovaSlider::OnIncrement()
{
	float NewValue = CurrentValue + ValueStep;
	NewValue       = FMath::Clamp(NewValue, Slider->GetMinValue(), Slider->GetMaxValue());
	CurrentValue   = NewValue;
	ValueChanged.ExecuteIfBound(CurrentValue);
}

void SNovaSlider::OnDecrement()
{
	float NewValue = CurrentValue - ValueStep;
	NewValue       = FMath::Clamp(NewValue, Slider->GetMinValue(), Slider->GetMaxValue());
	CurrentValue   = NewValue;
	ValueChanged.ExecuteIfBound(CurrentValue);
}

void SNovaSlider::OnMouseCaptured()
{
	IsMouseCaptured = true;
}

void SNovaSlider::OnMouseReleased()
{
	IsMouseCaptured = false;
}
