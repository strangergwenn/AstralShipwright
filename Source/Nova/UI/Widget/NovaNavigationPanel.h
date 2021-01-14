#pragma once

#include "Nova/Nova.h"
#include "Nova/UI/NovaUI.h"
#include "NovaButton.h"

#include "Widgets/SCompoundWidget.h"


/** Equivalent to SNew, automatically store the button into FocusableButtons */
#define SNovaNew(WidgetType, ...) \
	NewFocusableButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), false ) \
	<<= TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SNew, automatically store the button into FocusableButtons and set as default focus */
#define SNovaDefaultNew(WidgetType, ...) \
	NewFocusableButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), true ) \
	<<= TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SAssignNew, automatically store the button into FocusableButtons */
#define SNovaAssignNew(ExposeAs, WidgetType, ...) \
	NewFocusableButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), false ) \
	.Expose(ExposeAs) \
	<<= TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()

/** Equivalent to SAssignNew, automatically store the button into FocusableButtons and set as default focus */
#define SNovaDefaultAssignNew(ExposeAs, WidgetType, ...) \
	NewFocusableButton<WidgetType>(#WidgetType, __FILE__, __LINE__, RequiredArgs::MakeRequiredArgs(__VA_ARGS__), true ) \
	.Expose(ExposeAs) \
	<<= TYPENAME_OUTSIDE_TEMPLATE WidgetType::FArguments()


/** Inherit this class for focus support, set DefaultNavigationButton for default focus (optional) */
class SNovaNavigationPanel : public SCompoundWidget
{
	/*----------------------------------------------------
		Slate arguments
	----------------------------------------------------*/
	
	SLATE_BEGIN_ARGS(SNovaNavigationPanel)
		: _Menu(nullptr)
	{}

	SLATE_ARGUMENT(class SNovaMenu*, Menu)

	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

public:

	/*----------------------------------------------------
		Interaction
	----------------------------------------------------*/

	/** Create a new focusable button */
	template<typename WidgetType, typename RequiredArgsPayloadType>
	TDecl<WidgetType, RequiredArgsPayloadType> NewFocusableButton(const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine,
		RequiredArgsPayloadType&& InRequiredArgs, bool DefaultFocus)
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

	/** Pass the primary ability input to this menu */
	virtual void AbilityPrimary();

	/** Pass the secondary ability input to this menu */
	virtual void AbilitySecondary();

	/** Zoom in */
	virtual void ZoomIn();

	/** Zoom out */
	virtual void ZoomOut();

	/** Pass the confirm input to this menu, return true if used */
	virtual bool Confirm();

	/** Pass the cancel input to this menu, return true if used */
	virtual bool Cancel();

	/** Pass horizontal right-stick or mouse drag input to this widget */
	virtual void HorizontalAnalogInput(float Value)
	{}

	/** Pass vertical right-stick or mouse drag input to this widget */
	virtual void VerticalAnalogInput(float Value)
	{}

	/** Focus changed to this button */
	virtual void OnFocusChanged(TSharedPtr<class SNovaButton> FocusButton)
	{}

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
		ChildSlot
			[
				InContent
			];
	}


protected:

	class SNovaMenu*                              Menu;
	TSharedPtr<SNovaButton>                       DefaultNavigationButton;
	TArray<TSharedPtr<SNovaButton>>               NavigationButtons;

};
