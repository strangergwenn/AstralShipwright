// Astral Shipwright - Gwennaël Arbona

#include "NovaNotification.h"

#include "Nova.h"

#include "Neutron/System/NeutronMenuManager.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "SNovaNotification"

/*----------------------------------------------------
    Notification widget
----------------------------------------------------*/

void SNovaNotification::Construct(const FArguments& InArgs)
{
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	// Settings
	MenuManager = InArgs._MenuManager;

	// clang-format off
	SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments()
		.FadeDuration(ENeutronUIConstants::FadeDurationShort)
		.DisplayDuration(5.0f)
		.ColorAndOpacity(this, &SNovaNotification::GetNotifyColor)
	);

	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0, 200))
	[
		SNew(SBox)
		.WidthOverride(Theme.GenericMenuWidth)
		.Padding(0)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBorder)
			.ColorAndOpacity(this, &SNeutronFadingWidget::GetLinearColor)
			.BorderBackgroundColor(this, &SNeutronFadingWidget::GetSlateColor)
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

void SNovaNotification::Notify(const FText& Text, const FText& Subtext, ENeutronNotificationType Type)
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

	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	FSlateApplication::Get().PlaySound(Theme.NotifySound);
}

FLinearColor SNovaNotification::GetNotifyColor() const
{
	return 1.25f * MenuManager->GetHighlightColor();
}

const FSlateBrush* SNovaNotification::GetNotifyIcon() const
{
	if (DesiredNotifyType == ENeutronNotificationType::Info)
	{
		return FNeutronStyleSet::GetBrush("Icon/SB_Notify_Info");
	}
	else if (DesiredNotifyType == ENeutronNotificationType::Error)
	{
		return FNeutronStyleSet::GetBrush("Icon/SB_Notify_Error");
	}
	else if (DesiredNotifyType == ENeutronNotificationType::Save)
	{
		return FNeutronStyleSet::GetBrush("Icon/SB_Notify_Save");
	}
	else if (DesiredNotifyType == ENeutronNotificationType::World)
	{
		return FNeutronStyleSet::GetBrush("Icon/SB_Notify_World");
	}
	else if (DesiredNotifyType == ENeutronNotificationType::Time)
	{
		return FNeutronStyleSet::GetBrush("Icon/SB_Notify_Time");
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
