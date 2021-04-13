// Nova project - Gwennaël Arbona

#include "NovaSlider.h"
#include "NovaButton.h"
#include "NovaNavigationPanel.h"
#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

/*----------------------------------------------------
    Button class that steals analog input when focused
----------------------------------------------------*/

class SNovaSliderButton : public SNovaButton
{
	SLATE_BEGIN_ARGS(SNovaSliderButton)
	{}

	SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ARGUMENT(FName, Theme)
	SLATE_ARGUMENT(FName, Size)

	SLATE_ATTRIBUTE(bool, Enabled)

	SLATE_EVENT(FSimpleDelegate, OnClicked)

	SLATE_ARGUMENT(class SNovaSlider*, Slider)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Slider = InArgs._Slider;

		SNovaButton::Construct(SNovaButton::FArguments()
								   .Icon(InArgs._Icon)
								   .HelpText(InArgs._HelpText)
								   .Theme(InArgs._Theme)
								   .Size(InArgs._Size)
								   .Enabled(InArgs._Enabled)
								   .OnClicked(InArgs._OnClicked));
	}

	virtual bool HorizontalAnalogInput(float Value) override
	{
		Slider->HorizontalAnalogInput(Value);
		return true;
	}

	virtual bool VerticalAnalogInput(float Value) override
	{
		return true;
	}

protected:
	SNovaSlider* Slider;
};

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaSlider::Construct(const FArguments& InArgs)
{
	// Arguments
	ThemeName    = InArgs._Theme;
	EnabledState = InArgs._Enabled;
	ValueStep    = InArgs._ValueStep;
	Analog       = InArgs._Analog;
	CurrentValue = InArgs._Value;
	ValueChanged = InArgs._OnValueChanged;

	// Setup
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();
	const FNovaSliderTheme& Theme       = FNovaStyleSet::GetTheme<FNovaSliderTheme>(ThemeName);
	const FNovaSliderSize&  Size        = FNovaStyleSet::GetTheme<FNovaSliderSize>(InArgs._Size);
	SliderSpeed                         = 1.0f;
	SliderAnalogSpeed                   = 0.01f;

	// clang-format off
	ChildSlot
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(Size.Width)
		[
			SNew(SHorizontalBox)

			// Decrement button
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				InArgs._Panel->SNovaNew(SNovaSliderButton)
				.HelpText(InArgs._HelpText)
				.Theme(InArgs._ControlsTheme)
				.Size(InArgs._ControlsSize)
				.Icon(FNovaStyleSet::GetBrush("Icon/SB_Minus"))
				.OnClicked(this, &SNovaSlider::OnDecrement)
				.Enabled(InArgs._Enabled)
				.Slider(this)
			]

			// Main brush
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(Theme.Padding)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(Theme.SliderStyle.NormalThumbImage.GetImageSize().X, 0))
				[
					InArgs._Header.Widget
				]
			
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.Border)
					.Padding(FMargin(1))
					.ColorAndOpacity(this, &SNovaSlider::GetColor)
					.BorderBackgroundColor(this, &SNovaSlider::GetSlateColor)
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

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(Theme.SliderStyle.NormalThumbImage.GetImageSize().X, 0))
				[
					InArgs._Footer.Widget
				]
			]

			// Increment button
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				InArgs._Panel->SNovaNew(SNovaSliderButton)
				.HelpText(InArgs._HelpText)
				.Theme(InArgs._ControlsTheme)
				.Size(InArgs._ControlsSize)
				.Icon(FNovaStyleSet::GetBrush("Icon/SB_Plus"))
				.OnClicked(this, &SNovaSlider::OnIncrement)
				.Enabled(InArgs._Enabled)
				.Slider(this)
			]
		]
	];
	// clang-format on
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
		float Range = Slider->GetMaxValue() - Slider->GetMinValue();
		Delta       = FMath::Clamp(Delta, -SliderSpeed * Range * DeltaTime, SliderSpeed * Range * DeltaTime);

		Slider->SetValue(Slider->GetValue() + Delta);
	}
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

void SNovaSlider::HorizontalAnalogInput(float Value)
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
	const FNovaSliderTheme& Theme = FNovaStyleSet::GetTheme<FNovaSliderTheme>(ThemeName);

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
