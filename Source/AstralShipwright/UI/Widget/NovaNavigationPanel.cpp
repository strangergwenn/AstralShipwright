#pragma once

#include "NovaNavigationPanel.h"
#include "NovaMenu.h"
#include "Nova.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaNavigationPanel::Construct(const FArguments& InArgs)
{
	Menu = InArgs._Menu;

	ChildSlot[InArgs._Content.Widget];
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaNavigationPanel::Next()
{}

void SNovaNavigationPanel::Previous()
{}

TSharedPtr<SNovaButton> SNovaNavigationPanel::GetDefaultFocusButton() const
{
	if (DefaultNavigationButton && DefaultNavigationButton->SupportsKeyboardFocus())
	{
		return DefaultNavigationButton;
	}
	else
	{
		for (TSharedPtr<SNovaButton> Button : NavigationButtons)
		{
			if (Button->SupportsKeyboardFocus())
			{
				return Button;
			}
		}
	}

	return nullptr;
}

bool SNovaNavigationPanel::IsButtonActionAllowed(TSharedPtr<SNovaButton> Button) const
{
	TSharedPtr<SWidget> ButtonParentWidget = Button->GetParentWidget();
	while (ButtonParentWidget.IsValid() && ButtonParentWidget.Get() != this)
	{
		ButtonParentWidget = ButtonParentWidget->GetParentWidget();
	}

	return ButtonParentWidget.IsValid();
}

TArray<TSharedPtr<SNovaButton>>& SNovaNavigationPanel::GetNavigationButtons()
{
	return NavigationButtons;
}

void SNovaNavigationPanel::ResetNavigation()
{
	if (Menu)
	{
		TSharedPtr<SNovaButton> FocusButton = GetDefaultFocusButton();
		if (FocusButton && FocusButton->SupportsKeyboardFocus())
		{
			// NLOG("SNovaNavigationPanel::ResetNavigation : reset to '%s'", *FocusButton->ToString());
			Menu->SetFocusedButton(FocusButton, true);
		}
		else
		{
			Menu->SetFocusedButton(nullptr, true);
		}
	}
}

bool SNovaNavigationPanel::IsFocusedButtonInsideWidget(TSharedPtr<SWidget> Widget) const
{
	return Menu ? IsButtonInsideWidget(Menu->GetFocusedButton(), Widget) : false;
}

bool SNovaNavigationPanel::IsButtonInsideWidget(TSharedPtr<SNovaButton> Button, TSharedPtr<SWidget> Widget) const
{
	if (Button.IsValid() && NavigationButtons.Contains(Button))
	{
		const FVector2D& OriginPoint = Button->GetCachedGeometry().GetAbsolutePosition();
		const FVector2D& Size        = Button->GetCachedGeometry().GetAbsoluteSize();
		if (Widget->GetCachedGeometry().GetLayoutBoundingRect().ContainsPoint(OriginPoint) ||
			Widget->GetCachedGeometry().GetLayoutBoundingRect().ContainsPoint(OriginPoint + Size))
		{
			return true;
		}
	}

	return false;
}
