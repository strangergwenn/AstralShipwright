// Nova project - Gwennaël Arbona

#include "NovaMenu.h"
#include "NovaButton.h"
#include "NovaNavigationPanel.h"
#include "NovaModalPanel.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#include "GameFramework/InputSettings.h"
#include "Input/HittestGrid.h"

#define LOCTEXT_NAMESPACE "SNovaMenu"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMenu::SNovaMenu()
	: AnalogNavThreshold(0.25f)
	, AnalogNavMinPeriod(0.20f)
	, AnalogNavMaxPeriod(1.0f)
	, MousePressed(false)
	, MousePressedContinued(false)
	, CurrentNavigationPanel(nullptr)
	, CurrentAnalogNavigation(EUINavigation::Invalid)
	, CurrentAnalogNavigationTime(0)
{}

void SNovaMenu::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;
	NCHECK(MenuManager.IsValid());

	// clang-format off
	ChildSlot
	[
		SAssignNew(MainOverlay, SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(MainContainer, SBox)
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMenu::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Reset navigation if nothing is focused
	if (CurrentNavigationPanel && !GetFocusedButton())
	{
		CurrentNavigationPanel->ResetNavigation();
	}

	// Update analog navigation
	CurrentAnalogNavigationTime += DeltaTime;

	// Update analog input processing
	if (CurrentNavigationPanel)
	{
		if (!MenuManager->IsUsingGamepad())
		{
			CurrentAnalogInput = FVector2D::ZeroVector;
		}

		if (MousePressed)
		{
			auto&     App       = FSlateApplication::Get();
			FVector2D CursorPos = App.GetCursorPos();

			// Compute the current analog mouse input with axis snapping
			if (MousePressedContinued && CursorPos != PreviousMousePosition)
			{
				FVector2D MouseDirection = (CursorPos - PreviousMousePosition).GetSafeNormal();

				float AxisDominanceThreshold = 0.5f;
				if (FMath::Abs(MouseDirection.X) - FMath::Abs(MouseDirection.Y) > AxisDominanceThreshold)
				{
					MouseDirection = FVector2D(FMath::Sign(MouseDirection.X), 0);
				}
				else if (FMath::Abs(MouseDirection.Y) - FMath::Abs(MouseDirection.X) > AxisDominanceThreshold)
				{
					MouseDirection = FVector2D(0, FMath::Sign(MouseDirection.Y));
				}

				CurrentAnalogInput.X = MouseDirection.X;
				CurrentAnalogInput.Y = -MouseDirection.Y;
			}

			PreviousMousePosition = CursorPos;
			MousePressedContinued = true;
		}
		else
		{
			MousePressedContinued = false;
		}

		// Process analog input
		float ConstantRateRatio = DeltaTime * 60.0f;
		float HorizontalInput   = ConstantRateRatio * CurrentAnalogInput.X;
		float VerticalInput     = ConstantRateRatio * CurrentAnalogInput.Y;

		// Pass analog input when using gamepad
		bool                    HorizontalInputConsumed = false;
		bool                    VerticalInputConsumed   = false;
		TSharedPtr<SNovaButton> Button                  = GetFocusedButton();
		if (MenuManager->IsUsingGamepad() && Button.IsValid())
		{
			HorizontalInputConsumed = Button->HorizontalAnalogInput(HorizontalInput);
			VerticalInputConsumed   = Button->VerticalAnalogInput(VerticalInput);
		}
		if (!HorizontalInputConsumed)
		{
			CurrentNavigationPanel->HorizontalAnalogInput(HorizontalInput);
		}
		if (!VerticalInputConsumed)
		{
			CurrentNavigationPanel->VerticalAnalogInput(VerticalInput);
		}
	}
}

bool SNovaMenu::SupportsKeyboardFocus() const
{
	return true;
}

FReply SNovaMenu::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		MousePressed = true;
	}

	MenuManager->SetUsingGamepad(false);

	if (CurrentNavigationPanel)
	{
		FVector2D Position = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		CurrentNavigationPanel->OnClicked(Position);
	}

	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

FReply SNovaMenu::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);

	MousePressed = false;

	HandleKeyPress(MouseEvent.GetEffectingButton());

	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

FReply SNovaMenu::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Result = SCompoundWidget::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);

	if (CurrentNavigationPanel)
	{
		FVector2D Position = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		CurrentNavigationPanel->OnDoubleClicked(Position);
	}

	return Result;
}

FReply SNovaMenu::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Result = SCompoundWidget::OnMouseWheel(MyGeometry, MouseEvent);

	return HandleKeyPress(MouseEvent.GetWheelDelta() > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
}

void SNovaMenu::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	MousePressed = false;
}

FReply SNovaMenu::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent)
{
	SCompoundWidget::OnAnalogValueChanged(MyGeometry, AnalogInputEvent);

	// Get data
	const FKey              Key           = AnalogInputEvent.GetKey();
	FReply                  Result        = FReply::Unhandled();
	TSharedPtr<SNovaButton> FocusedButton = GetFocusedButton();
	TSharedPtr<SNovaButton> DestinationButton;

	// Handle menu keys
	if (CurrentNavigationPanel)
	{
		EUINavigation AnalogNavigation   = EUINavigation::Invalid;
		float         CurrentInputPeriod = FMath::Lerp(AnalogNavMaxPeriod, AnalogNavMinPeriod,
            (FMath::Abs(AnalogInputEvent.GetAnalogValue()) - AnalogNavThreshold) / (1.0f - AnalogNavThreshold));

		// Handle navigation
		if (IsAxisKey(FNovaPlayerInput::MenuMoveHorizontal, Key))
		{
			if (-AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Left;
			}
			else if (AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Right;
			}
		}
		else if (IsAxisKey(FNovaPlayerInput::MenuMoveVertical, Key))
		{
			if (AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Up;
			}
			else if (-AnalogInputEvent.GetAnalogValue() > AnalogNavThreshold)
			{
				AnalogNavigation = EUINavigation::Down;
			}
		}

		// Update focus destination with a maximum period
		if (AnalogNavigation != EUINavigation::Invalid && CurrentAnalogNavigationTime >= CurrentInputPeriod)
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, AnalogNavigation);
			if (DestinationButton.IsValid() && DestinationButton->SupportsKeyboardFocus())
			{
				SetFocusedButton(DestinationButton, true);

				CurrentAnalogNavigation     = AnalogNavigation;
				CurrentAnalogNavigationTime = 0;
				Result                      = FReply::Handled();
			}
		}
	}

	auto InputFilter = [&](float InputValue)
	{
		return FMath::Sign(InputValue) * FMath::Max(FMath::Abs(InputValue) - AnalogNavThreshold, 0.0f) / (1.0f - AnalogNavThreshold);
	};

	// Read analog input from controller axis
	if (IsAxisKey(FNovaPlayerInput::MenuAnalogHorizontal, AnalogInputEvent.GetKey()))
	{
		CurrentAnalogInput.X = InputFilter(AnalogInputEvent.GetAnalogValue());
	}
	else if (IsAxisKey(FNovaPlayerInput::MenuAnalogVertical, AnalogInputEvent.GetKey()))
	{
		CurrentAnalogInput.Y = InputFilter(AnalogInputEvent.GetAnalogValue());
	}

	return Result;
}

FReply SNovaMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	SCompoundWidget::OnKeyDown(MyGeometry, KeyEvent);

	// Get data
	const FKey              Key           = KeyEvent.GetKey();
	FReply                  Result        = FReply::Unhandled();
	TSharedPtr<SNovaButton> FocusedButton = GetFocusedButton();

	// Set gamepad used
	MenuManager->SetUsingGamepad(Key.IsGamepadKey());

	// Pass the event down
	if (FocusedButton.IsValid())
	{
		Result = FocusedButton->OnKeyDown(FocusedButton->GetCachedGeometry(), KeyEvent);
	}

	return HandleKeyPress(Key);
}

/*----------------------------------------------------
    Input handling
----------------------------------------------------*/

void SNovaMenu::UpdateKeyBindings()
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	NCHECK(InputSettings);

	// Update action bindings
	ActionBindings.Reset();
	for (int32 i = 0; i < InputSettings->GetActionMappings().Num(); i++)
	{
		FInputActionKeyMapping Action = InputSettings->GetActionMappings()[i];
		ActionBindings.Add(Action.Key.GetFName(), Action.ActionName);
	}

	// Update axis bindings
	AxisBindings.Reset();
	for (int32 i = 0; i < InputSettings->GetAxisMappings().Num(); i++)
	{
		FInputAxisKeyMapping Axis = InputSettings->GetAxisMappings()[i];
		AxisBindings.Add(Axis.Key.GetFName(), Axis.AxisName);
	}
}

bool SNovaMenu::IsActionKey(FName ActionName, const FKey& Key) const
{
	return ActionBindings.FindPair(Key.GetFName(), ActionName) != nullptr;
}

bool SNovaMenu::IsAxisKey(FName AxisName, const FKey& Key) const
{
	return AxisBindings.FindPair(Key.GetFName(), AxisName) != nullptr;
}

/*----------------------------------------------------
    Focus handling
----------------------------------------------------*/

void SNovaMenu::SetNavigationPanel(SNovaNavigationPanel* Panel)
{
	// NLOG("SNovaMenu::SetNavigationPanel : '%s'", Panel ? *Panel->GetTypeAsString() : TEXT("nullptr"));

	NCHECK(Panel);

	if (Panel != CurrentNavigationPanel)
	{
		NCHECK(CurrentNavigationButtons.Num() == 0);
		NCHECK(CurrentNavigationPanel == nullptr);

		CurrentNavigationPanel   = Panel;
		CurrentNavigationButtons = Panel->GetNavigationButtons();
	}
	else
	{
		RefreshNavigationPanel();
	}
}

void SNovaMenu::ClearNavigationPanel()
{
	// NLOG("SNovaMenu::ClearNavigationPanel : '%s'", CurrentNavigationPanel ? *CurrentNavigationPanel->GetTypeAsString() :
	// TEXT("nullptr"));

	for (TSharedPtr<SNovaButton> Button : CurrentNavigationButtons)
	{
		Button->SetFocused(false);
	}

	CurrentNavigationPanel = nullptr;
	CurrentNavigationButtons.Empty();
}

void SNovaMenu::RefreshNavigationPanel()
{
	if (CurrentNavigationPanel)
	{
		CurrentNavigationButtons = CurrentNavigationPanel->GetNavigationButtons();
	}
}

TSharedPtr<SNovaModalPanel> SNovaMenu::CreateModalPanel()
{
	TSharedPtr<SNovaModalPanel> Panel;

	MainOverlay->AddSlot()[SAssignNew(Panel, SNovaModalPanel).Menu(this)];

	return Panel;
}

void SNovaMenu::SetModalNavigationPanel(class SNovaNavigationPanel* Panel)
{
	NCHECK(CurrentNavigationPanel);

	PreviousNavigationPanel = CurrentNavigationPanel;

	ClearNavigationPanel();
	SetNavigationPanel(Panel);
}

void SNovaMenu::ClearModalNavigationPanel()
{
	NCHECK(PreviousNavigationPanel);

	ClearNavigationPanel();
	SetNavigationPanel(PreviousNavigationPanel);

	PreviousNavigationPanel = nullptr;
}

void SNovaMenu::SetFocusedButton(TSharedPtr<SNovaButton> FocusButton, bool FromNavigation)
{
	if (CurrentNavigationButtons.Contains(FocusButton) && FocusButton->SupportsKeyboardFocus())
	{
		for (TSharedPtr<SNovaButton> Button : CurrentNavigationButtons)
		{
			if (Button != FocusButton && Button->IsFocused())
			{
				Button->SetFocused(false);
			}
		}

		FocusButton->SetFocused(true);

		if (FromNavigation)
		{
			CurrentNavigationPanel->OnFocusChanged(FocusButton);
		}
	}
	else if (!FocusButton.IsValid())
	{
		for (TSharedPtr<SNovaButton> Button : CurrentNavigationButtons)
		{
			if (Button != FocusButton && Button->IsFocused())
			{
				Button->SetFocused(false);
			}
		}

		if (FromNavigation)
		{
			CurrentNavigationPanel->OnFocusChanged(FocusButton);
		}
	}
}

TSharedPtr<SNovaButton> SNovaMenu::GetFocusedButton()
{
	for (TSharedPtr<SNovaButton> Button : CurrentNavigationButtons)
	{
		if (Button->IsFocused() && Button->IsButtonEnabled())
		{
			return Button;
		}
	}

	return TSharedPtr<SNovaButton>();
}

FReply SNovaMenu::HandleKeyPress(FKey Key)
{
	FReply                  Result        = FReply::Unhandled();
	TSharedPtr<SNovaButton> FocusedButton = GetFocusedButton();
	TSharedPtr<SNovaButton> DestinationButton;

	// Handle menu keys
	if (CurrentNavigationPanel)
	{
		// Handle navigation
		if (IsActionKey(FNovaPlayerInput::MenuUp, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Up);
		}
		else if (IsActionKey(FNovaPlayerInput::MenuDown, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Down);
		}
		else if (IsActionKey(FNovaPlayerInput::MenuLeft, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Left);
		}
		else if (IsActionKey(FNovaPlayerInput::MenuRight, Key))
		{
			DestinationButton =
				GetNextButton(SharedThis(CurrentNavigationPanel), FocusedButton, CurrentNavigationButtons, EUINavigation::Right);
		}

		// Handle menu actions
		else if (IsActionKey(FNovaPlayerInput::MenuNext, Key))
		{
			CurrentNavigationPanel->Next();
			Result = FReply::Handled();
		}
		else if (IsActionKey(FNovaPlayerInput::MenuPrevious, Key))
		{
			CurrentNavigationPanel->Previous();
			Result = FReply::Handled();
		}
	}

	// Update focus destination
	if (DestinationButton.IsValid() && DestinationButton->SupportsKeyboardFocus())
	{
		SetFocusedButton(DestinationButton, true);
		Result = FReply::Handled();
	}

	// Cancel action has priority over remaining actions but can be ignored
	if (IsActionKey(FNovaPlayerInput::MenuCancel, Key))
	{
		if (CurrentNavigationPanel && CurrentNavigationPanel->Cancel())
		{
			Result = FReply::Handled();
			return Result;
		}
	}

	// Trigger action buttons
	for (TSharedPtr<SNovaButton>& Button : GetActionButtons())
	{
		if (Button->GetActionKey() == Key && Button->IsButtonEnabled() && Button->GetVisibility() == EVisibility::Visible)
		{
			bool WasFocused = Button->IsFocused();

			Button->OnButtonClicked();

			if (CurrentNavigationPanel && WasFocused && Button->IsButtonActionFocusable())
			{
				CurrentNavigationPanel->ResetNavigation();
			}

			Result = FReply::Handled();
			break;
		}
	}

	// Activate focused button
	if (IsActionKey(FNovaPlayerInput::MenuConfirm, Key))
	{
		if (CurrentNavigationPanel && CurrentNavigationPanel->Confirm())
		{
			Result = FReply::Handled();
		}
		else if (FocusedButton.IsValid())
		{
			FocusedButton->OnButtonClicked();
			Result = FReply::Handled();
		}
	}

	return Result;
}

TSharedPtr<SNovaButton> SNovaMenu::GetNextButton(
	TSharedRef<SWidget> Widget, TSharedPtr<const SWidget> Current, TArray<TSharedPtr<SNovaButton>> Candidates, EUINavigation Direction)
{
	if (Current)
	{
		TSharedPtr<SNovaButton> Result = GetNextButtonInternal(Widget, Current, Candidates, Direction);
		if (!Result)
		{
			Result = GetNextButtonInternal(Widget, Current->GetParentWidget(), Candidates, Direction);
		}

		return Result;
	}
	else
	{
		return TSharedPtr<SNovaButton>();
	}
}

TSharedPtr<SNovaButton> SNovaMenu::GetNextButtonInternal(
	TSharedRef<SWidget> Widget, TSharedPtr<const SWidget> Current, TArray<TSharedPtr<SNovaButton>> Candidates, EUINavigation Direction)
{
	FWidgetPath Source;
	FWidgetPath Boundary;

	// Find the next widget in the required direction within the tab view
	if (Current.IsValid())
	{
		if (FSlateApplication::Get().FindPathToWidget(Current.ToSharedRef(), Source) &&
			FSlateApplication::Get().FindPathToWidget(Widget, Boundary))
		{
			FNavigationReply       NavigationReply = FNavigationReply::Explicit(Widget);
			const FArrangedWidget& SourceWidget    = Source.Widgets.Last();
			const FArrangedWidget& BoundaryWidget  = Boundary.Widgets.Last();

			TSharedPtr<SWidget> DestinationWidget = Source.GetWindow()->GetHittestGrid().FindNextFocusableWidget(
				SourceWidget, Direction, NavigationReply, BoundaryWidget, FSlateApplication::Get().CursorUserIndex);

			if (DestinationWidget.IsValid() && Candidates.Contains(DestinationWidget))
			{
				SNovaButton* DestinationButton = static_cast<SNovaButton*>(DestinationWidget.Get());
				if (DestinationButton->IsButtonEnabled())
				{
					return SharedThis(DestinationButton);
				}
			}
		}
	}

	return TSharedPtr<SNovaButton>();
}

#undef LOCTEXT_NAMESPACE
