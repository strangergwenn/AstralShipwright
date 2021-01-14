// Nova project - Gwennaël Arbona

#include "NovaSlider.h"
#include "NovaButton.h"
#include "NovaNavigationPanel.h"
#include "Nova/Nova.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"


/*----------------------------------------------------
	Construct
----------------------------------------------------*/

void SNovaSlider::Construct(const FArguments& InArgs)
{
	// Arguments
	ThemeName = InArgs._Theme;
	EnabledState = InArgs._Enabled;
	ValueStep = InArgs._ValueStep;
	CurrentValue = InArgs._Value;
	ValueChanged = InArgs._OnValueChanged;

	// Setup
	const FNovaSliderTheme& Theme = FNovaStyleSet::GetTheme<FNovaSliderTheme>(ThemeName);
	SliderSpeed = 5.0f;

	// Structure
	ChildSlot
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(Theme.Width)
		[
			SNew(SHorizontalBox)

			// Decrement button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				InArgs._Panel->SNovaNew(SNovaButton)
				.HelpText(InArgs._HelpText)
				.Theme(InArgs._ControlsTheme)
				.Size(InArgs._ControlsSize)
				.Icon(FNovaStyleSet::GetBrush("Icon/SB_Minus"))
				.OnClicked(this, &SNovaSlider::OnDecrement)
				.Enabled(InArgs._Enabled)
			]

			// Main brush
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(Theme.Padding)
			[
				SNew(SBorder)
				.BorderImage(&Theme.Border)
				.Padding(FMargin(1))
				[
					SNew(SBorder)
					.BorderImage(this, &SNovaSlider::GetBackgroundBrush)
					.Padding(0)
					[
						SAssignNew(Slider, SSlider)
						.IsFocusable(false)
						.Value(InArgs._Value)
						.MinValue(InArgs._MinValue)
						.MaxValue(InArgs._MaxValue)
						.OnValueChanged(this, &SNovaSlider::OnSliderValueChanged)
						.OnMouseCaptureBegin(this, &SNovaSlider::OnMouseCaptured)
						.OnMouseCaptureEnd(this, &SNovaSlider::OnMouseReleased)
						.Style(&Theme.SliderStyle)
						.SliderBarColor(FLinearColor(0, 0, 0, 0))
					]
				]
			]

			// Increment button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				InArgs._Panel->SNovaNew(SNovaButton)
				.HelpText(InArgs._HelpText)
				.Theme(InArgs._ControlsTheme)
				.Size(InArgs._ControlsSize)
				.Icon(FNovaStyleSet::GetBrush("Icon/SB_Plus"))
				.OnClicked(this, &SNovaSlider::OnIncrement)
				.Enabled(InArgs._Enabled)
			]
		]
	];
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaSlider::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	if (Slider->GetValue() != CurrentValue)
	{
		float Delta = CurrentValue - Slider->GetValue();
		Delta = FMath::Clamp(Delta, -SliderSpeed * DeltaTime, SliderSpeed * DeltaTime);

		Slider->SetValue(Slider->GetValue() + Delta);
	}
}

float SNovaSlider::GetCurrentValue() const
{
	return CurrentValue;
}

void SNovaSlider::SetCurrentValue(float Value)
{
	CurrentValue = Value;
	Slider->SetValue(CurrentValue);
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

const FSlateBrush* SNovaSlider::GetBackgroundBrush() const
{
	const FNovaSliderTheme& Theme = FNovaStyleSet::GetTheme<FNovaSliderTheme>(ThemeName);

	if (!IsEnabled())
	{
		return &Theme.SliderStyle.DisabledBarImage;
	}
	else if (Slider->IsHovered() || IsMouseCaptured)
	{
		return &Theme.SliderStyle.HoveredBarImage;
	}
	else
	{
		return &Theme.SliderStyle.NormalBarImage;
	}
}

void SNovaSlider::OnIncrement()
{
	float NewValue = Slider->GetValue() + ValueStep;
	NewValue = FMath::Clamp(NewValue, Slider->GetMinValue(), Slider->GetMaxValue());
	CurrentValue = NewValue;
	ValueChanged.ExecuteIfBound(CurrentValue);
}

void SNovaSlider::OnDecrement()
{
	float NewValue = Slider->GetValue() - ValueStep;
	NewValue = FMath::Clamp(NewValue, Slider->GetMinValue(), Slider->GetMaxValue());
	CurrentValue = NewValue;
	ValueChanged.ExecuteIfBound(CurrentValue);
}

void SNovaSlider::OnSliderValueChanged(float Value)
{
	float StepCount = 1.0f / ValueStep;
	float StepValue = FMath::RoundToInt(StepCount * Value);
	float RoundedValue = StepValue / StepCount;

	if (RoundedValue != CurrentValue)
	{
		ValueChanged.ExecuteIfBound(RoundedValue);
		CurrentValue = RoundedValue;
	}

	if (IsMouseCaptured)
	{
		Slider->SetValue(RoundedValue);
	}
}

void SNovaSlider::OnMouseCaptured()
{
	IsMouseCaptured = true;
}

void SNovaSlider::OnMouseReleased()
{
	IsMouseCaptured = false;
}
