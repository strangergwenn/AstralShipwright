// Nova project - Gwennaël Arbona

#include "NovaNotification.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaNotification"

/*----------------------------------------------------
    Notification widget
----------------------------------------------------*/

void SNovaNotification::Construct(const FArguments& InArgs)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Settings
	MenuManager = InArgs._MenuManager;

	// clang-format off
	SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
		.FadeDuration(ENovaUIConstants::FadeDurationShort)
		.DisplayDuration(5.0f)
	);

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(Theme.NotificationDisplayWidth)
		.Padding(0)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBorder)
			.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
			.BorderBackgroundColor(this, &SNovaNotification::GetNotifyColor)
			.Padding(2)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(Theme.BlurRadius)
				.BlurStrength(Theme.BlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.HAlign(HAlign_Center)
					.BorderImage(&Theme.MainMenuPatternedBackground)
					.Padding(Theme.ContentPadding)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(this, &SNovaNotification::GetNotifyIcon)
							.ColorAndOpacity(this, &SNovaNotification::GetNotifyColor)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(this, &SNovaNotification::GetNotifyText)
							.TextStyle(&Theme.NotificationFont)
							.ColorAndOpacity(this, &SNovaNotification::GetNotifyColor)
						]
					]
				]
			]
		]
	];
	// clang-format on
}

void SNovaNotification::Notify(const FText& Text, ENovaNotificationType Type)
{
	NLOG("SNovaNotification::Notify : %s", *Text.ToString());

	DesiredNotifyText = Text;
	DesiredNotifyType = Type;

	// Force update if we're past the original time
	if (CurrentDisplayTime > DisplayDuration)
	{
		CurrentNotifyText = FText();
	}
}

FSlateColor SNovaNotification::GetNotifyColor() const
{
	return 1.25f * SNovaFadingWidget::GetLinearColor() * MenuManager->GetHighlightColor();
}

const FSlateBrush* SNovaNotification::GetNotifyIcon() const
{
	if (DesiredNotifyType == ENovaNotificationType::Info)
	{
		return FNovaStyleSet::GetBrush("Icon/SB_Notify_Info");
	}
	else if (DesiredNotifyType == ENovaNotificationType::Warning)
	{
		return FNovaStyleSet::GetBrush("Icon/SB_Notify_Warning");
	}
	else if (DesiredNotifyType == ENovaNotificationType::Error)
	{
		return FNovaStyleSet::GetBrush("Icon/SB_Notify_Error");
	}
	else if (DesiredNotifyType == ENovaNotificationType::Save)
	{
		return FNovaStyleSet::GetBrush("Icon/SB_Notify_Save");
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
