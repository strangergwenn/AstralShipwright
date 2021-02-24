// Nova project - Gwennaël Arbona

#pragma once

#include "Nova/UI/Widget/NovaButton.h"
#include "Nova/UI/Widget/NovaKeyLabel.h"

/** Heavyweight button class */
class SNovaLargeButton : public SNovaButton
{
	SLATE_BEGIN_ARGS(SNovaLargeButton)
	{}

	SLATE_ATTRIBUTE(FText, Text)
	SLATE_ATTRIBUTE(FText, HelpText)
	SLATE_ATTRIBUTE(FName, Action)
	SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
	SLATE_EVENT(FSimpleDelegate, OnClicked)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme();

		// clang-format off
		SNovaButton::Construct(SNovaButton::FArguments()
			.Size("LargeButtonSize")
			.HelpText(InArgs._HelpText)
			.Action(InArgs._Action)
			.BorderRotation(45)
			.Header()
			[
				SNew(SOverlay)

				// Action
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.Padding(Theme.IconPadding)
				[
					SNew(SNovaKeyLabel)
					.Key(this, &SNovaLargeButton::GetActionKey)
				]

				// Text
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FMargin(0, 10))
				[
					SAssignNew(TextBlock, STextBlock)
					.TextStyle(&Theme.MainFont)
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

	FKey GetActionKey() const
	{
		UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
		NCHECK(MenuManager);

		return MenuManager->GetFirstActionKey(Action.Get());
	}
};
