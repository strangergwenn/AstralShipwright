#pragma once

#include "Nova/Nova.h"
#include "Nova/UI/NovaUI.h"
#include "NovaButton.h"

#include "Widgets/SCompoundWidget.h"

/** Inherit this class for focus support, set DefaultNavigationButton for default focus (optional) */
class SNovaNavigationPanel : public SCompoundWidget
{
	/*----------------------------------------------------
	    Slate arguments
	----------------------------------------------------*/

	SLATE_BEGIN_ARGS(SNovaNavigationPanel) : _Menu(nullptr)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)

	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/*----------------------------------------------------
	    Interaction
	----------------------------------------------------*/

public:
	/** Create a new focusable button */
	template <typename WidgetType, typename RequiredArgsPayloadType>
	TDecl<WidgetType, RequiredArgsPayloadType> NewNovaButton(
		const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine, RequiredArgsPayloadType&& InRequiredArgs, bool DefaultFocus)
	{
		auto Button = TDecl<WidgetType, RequiredArgsPayloadType>(InType, InFile, OnLine, Forward<RequiredArgsPayloadType>(InRequiredArgs));

		if ((NavigationButtons.Num() == 0 && Button._Widget->IsEnabled()) || DefaultFocus)
		{
			DefaultNavigationButton = Button._Widget;
		}
		NavigationButtons.Add(Button._Widget);

		return Button;
	}

	/** Release a focusable button */
	void DestroyFocusableButton(TSharedPtr<SNovaButton> Button)
	{
		NavigationButtons.Remove(Button);
	}

	/** Next item */
	virtual void Next();

	/** Previous item */
	virtual void Previous();

	/** Clicked */
	virtual void OnClicked(const FVector2D& Position)
	{}

	/** Clicked */
	virtual void OnDoubleClicked(const FVector2D& Position)
	{}

	/** Pass horizontal right-stick or mouse drag input to this widget */
	virtual void HorizontalAnalogInput(float Value)
	{}

	/** Pass vertical right-stick or mouse drag input to this widget */
	virtual void VerticalAnalogInput(float Value)
	{}

	/** Focus changed to this button */
	virtual void OnFocusChanged(TSharedPtr<class SNovaButton> FocusButton)
	{}

	/** Check whether this panel is modal */
	virtual bool IsModal() const
	{
		return false;
	}

	/** Get the ideal default focus button */
	virtual TSharedPtr<SNovaButton> GetDefaultFocusButton() const;

	/** Reset the focus to the default button, or any button */
	void ResetNavigation();

	/** Get all the buttons on this panel that we can navigate to */
	TArray<TSharedPtr<SNovaButton>>& GetNavigationButtons();

	/** Check if the focused button is valid and inside a widget of this panel */
	bool IsFocusedButtonInsideWidget(TSharedPtr<SWidget> Widget) const;

	/** Check if a particular button is valid and inside a widget of this panel */
	bool IsButtonInsideWidget(TSharedPtr<SNovaButton> Button, TSharedPtr<SWidget> Widget) const;

	/** Get the owner menu */
	class SNovaMenu* GetMenu() const
	{
		return Menu;
	}

	/** Set the contents */
	void SetContent(const TSharedRef<SWidget>& InContent)
	{
		ChildSlot[InContent];
	}

protected:
	// Navigation state
	class SNovaMenu*                Menu;
	TSharedPtr<SNovaButton>         DefaultNavigationButton;
	TArray<TSharedPtr<SNovaButton>> NavigationButtons;
};
