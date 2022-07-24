// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronButton.h"
#include "Neutron/UI/Widgets/NeutronKeyLabel.h"

/** Heavyweight button class */
class SNovaLargeButton : public SNeutronButton
{
	SLATE_BEGIN_ARGS(SNovaLargeButton)
	{}

	SLATE_ATTRIBUTE(FText, Text)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ARGUMENT(FName, Theme)
	SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
	SLATE_EVENT(FSimpleDelegate, OnClicked)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronButtonTheme& Theme = FNeutronStyleSet::GetButtonTheme(InArgs._Theme);

		// clang-format off
		SNeutronButton::Construct(SNeutronButton::FArguments()
			.Size("LargeButtonSize")
			.HelpText(InArgs._HelpText)
			.Action(InArgs._Action)
			.Theme(InArgs._Theme)
			.BorderRotation(45)
			.Header()
			[
				SNew(SOverlay)

				// Action
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.Padding(Theme.IconPadding)
				[
					SNew(SNeutronKeyLabel)
					.Action(this, &SNeutronButton::GetActionName)
				]

				// Text
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(0, 10))
				[
					SAssignNew(TextBlock, STextBlock)
					.TextStyle(&Theme.Font)
					.Text(InArgs._Text)
				]
			]
			.OnClicked(InArgs._OnClicked)
		);
		
		InnerContainer->SetContent(
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(InArgs._Icon)
			]
		);
		// clang-format on
	}
};
