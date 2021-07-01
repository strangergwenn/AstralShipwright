// Nova project - Gwennaël Arbona

#include "NovaNotification.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

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
		.ColorAndOpacity(this, &SNovaNotification::GetNotifyColor)
	);

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0, 200))
	[
		SNew(SBox)
		.WidthOverride(Theme.NotificationDisplayWidth)
		.Padding(0)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBorder)
			.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
			.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
			.Padding(2)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(Theme.BlurRadius)
				.BlurStrength(Theme.BlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuPatternedBackground)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Right)
							.Padding(Theme.ContentPadding)
							[
								SNew(SScaleBox)
								[
									SNew(SImage)
									.Image(this, &SNovaNotification::GetNotifyIcon)
								]
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SNovaNotification::GetNotifyText)
								.TextStyle(&Theme.NotificationFont)
							]

							+ SHorizontalBox::Slot()
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SNovaNotification::GetNotifySubtext)
							.TextStyle(&Theme.HeadingFont)
							.Visibility(this, &SNovaNotification::GetSubtextVisibility)
						]
					]
				]
			]
		]
	];
	// clang-format on
}

void SNovaNotification::Notify(const FText& Text, const FText& Subtext, ENovaNotificationType Type)
{
	NLOG("SNovaNotification::Notify : %s (%s)", *Text.ToString(), *Subtext.ToString());

	DesiredNotifyText    = Text;
	DesiredNotifySubtext = Subtext;
	DesiredNotifyType    = Type;

	// Force update if we're past the original time
	if (CurrentDisplayTime > DisplayDuration)
	{
		CurrentNotifyText = FText();
	}
}

FLinearColor SNovaNotification::GetNotifyColor() const
{
	return 1.25f * MenuManager->GetHighlightColor();
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
	else if (DesiredNotifyType == ENovaNotificationType::World)
	{
		return FNovaStyleSet::GetBrush("Icon/SB_Notify_World");
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
