#pragma once

#include "NovaKeyBinding.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Nova.h"

#include "GameFramework/InputSettings.h"


#define LOCTEXT_NAMESPACE "SNovaKeyBind"


/*----------------------------------------------------
	Key binding structure
----------------------------------------------------*/

FNovaKeyBinding* FNovaKeyBinding::Action(const FName& Mapping)
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	bool Found = false;

	// Find the binding
	for (const FInputActionKeyMapping& Action : InputSettings->GetActionMappings())
	{
		if (Mapping == Action.ActionName && !Action.Key.IsGamepadKey())
		{
			ActionMappings.Add(Action);

			if (UserKey == FKey())
			{
				UserKey = Action.Key;
			}

			Found = true;
		}
	}

	if (!Found)
	{
		FInputActionKeyMapping Action;
		Action.ActionName = Mapping;
		ActionMappings.Add(Action);
	}

	return this;
}

FNovaKeyBinding* FNovaKeyBinding::Axis(const FName& Mapping, float Scale)
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	bool Found = false;

	// Find the binding
	for (const FInputAxisKeyMapping& Axis : InputSettings->GetAxisMappings())
	{
		if (Mapping == Axis.AxisName && Axis.Scale == Scale && !Axis.Key.IsGamepadKey())
		{
			AxisMappings.Add(Axis);

			if (UserKey == FKey())
			{
				UserKey = Axis.Key;
			}

			Found = true;
		}
	}

	if (!Found)
	{
		FInputAxisKeyMapping Action;
		Action.AxisName = Mapping;
		Action.Scale = Scale;
		AxisMappings.Add(Action);
	}

	return this;
}

void FNovaKeyBinding::Save()
{
	UInputSettings* InputSettings = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();

	// Remove the original bindings
	for (auto& Bind : ActionMappings)
	{
		InputSettings->RemoveActionMapping(Bind);
	}
	for (auto& Bind : AxisMappings)
	{
		InputSettings->RemoveAxisMapping(Bind);
	}

	// Save action mappings
	TArray<FName> ActionMappingNames;
	for (FInputActionKeyMapping& Binding : ActionMappings)
	{
		Binding.Key = UserKey;
		InputSettings->AddActionMapping(Binding);
		ActionMappingNames.AddUnique(Binding.ActionName);
	}

	// Save axis mappings
	TArray<FName> AxisMappingNames;
	TArray<float> AxisMappingScales;
	for (FInputAxisKeyMapping& Binding : AxisMappings)
	{
		Binding.Key = UserKey;
		InputSettings->AddAxisMapping(Binding);

		AxisMappingNames.Add(Binding.AxisName);
		AxisMappingScales.Add(Binding.Scale);
	}

	// Reload action bindings
	ActionMappings.Empty();
	for (FName MappingName : ActionMappingNames)
	{
		Action(MappingName);
	}

	// Reload axis bindings
	AxisMappings.Empty();
	for (int i = 0; i < AxisMappingNames.Num(); i++)
	{
		Axis(AxisMappingNames[i], AxisMappingScales[i]);
	}
}


/*----------------------------------------------------
	Construct
----------------------------------------------------*/

void SNovaKeyBinding::Construct(const FArguments& InArgs)
{
	// Arguments
	Binding = InArgs._Binding;
	ThemeName = InArgs._Theme;
	OnKeyBindingChanged = InArgs._OnKeyBindingChanged;

	// Parent constructor
	SNovaButton::Construct(SNovaButton::FArguments()
		.Theme(InArgs._Theme)
		.Icon(FNovaStyleSet::GetBrush("Icon/SB_Edit"))
		.Text(this, &SNovaKeyBinding::GetKeyName)
		.HelpText(LOCTEXT("EditBinding", "Change this key binding"))
	);

	// Initialize
	WaitingForKey = false;
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaKeyBinding::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaButton::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Make sure the mouse pointer doesn't leave the button as long as we're waiting and focused
	if (WaitingForKey)
	{
		if (!IsFocused())
		{
			FinishWaiting();
		}
		else
		{
			FSlateApplication::Get().GetPlatformApplication()->Cursor->SetPosition(WaitingMousePos.X, WaitingMousePos.Y);
		}
	}
}

FReply SNovaKeyBinding::OnButtonClicked()
{
	// Get the center of the widget so we can lock our mouse there
	FSlateRect Rect = GetCachedGeometry().GetLayoutBoundingRect();
	WaitingMousePos.X = (Rect.Left + Rect.Right) * 0.5f;
	WaitingMousePos.Y = (Rect.Top + Rect.Bottom) * 0.5f;
	FSlateApplication::Get().GetPlatformApplication()->Cursor->SetPosition(WaitingMousePos.X, WaitingMousePos.Y);

	// Block input
	WaitingForKey = true;
	FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(false);

	return SNovaButton::OnButtonClicked();
}

void SNovaKeyBinding::OnKeyPicked(FKey NewKey, bool bCanReset, bool Notify)
{
	if (NewKey.IsValid())
	{
		// Save new key
		FKey CurrentKey = Binding->GetKey();
		if (NewKey == Binding->GetKey() && bCanReset)
		{
			NewKey = FKey();
		}
		Binding->SetKey(NewKey);

		// Resume input
		FinishWaiting();

		// Inform user
		if (Notify)
		{
			OnKeyBindingChanged.ExecuteIfBound(CurrentKey, NewKey);
		}
	}
}

void SNovaKeyBinding::FinishWaiting()
{
	WaitingForKey = false;
	FSlateApplication::Get().GetPlatformApplication()->Cursor->Show(true);
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

FText SNovaKeyBinding::GetKeyName() const
{
	if (WaitingForKey)
	{
		return (LOCTEXT("PressKey", "Press a key..."));
	}
	else
	{
		return Binding->GetKeyName();
	}
}

FReply SNovaKeyBinding::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (WaitingForKey && !InKeyEvent.GetKey().IsGamepadKey())
	{
		OnKeyPicked(InKeyEvent.GetKey());

		return FReply::Handled();
	}
	else
	{
		FinishWaiting();
		return FReply::Unhandled();
	}
}

FReply SNovaKeyBinding::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (WaitingForKey)
	{
		OnKeyPicked(MouseEvent.GetEffectingButton());

		return FReply::Handled();
	}

	// Super call is required for the button to even click
	return SNovaButton::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SNovaKeyBinding::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (WaitingForKey)
	{
		OnKeyPicked(MouseEvent.GetWheelDelta() > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}


#undef LOCTEXT_NAMESPACE
