// Nova project - Gwennaël Arbona

#include "NovaOverlay.h"
#include "NovaMainMenu.h"
#include "Nova/UI/Widget/NovaModalPanel.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"


#define LOCTEXT_NAMESPACE "SNovaOverlay"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

void SNovaOverlay::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager = InArgs._MenuManager;

	// Create material
	auto DynamicBrush = FNovaStyleSet::GetDynamicBrush("Notification/SB_NotificationIcon_Base");
	NotificationIconBrush = DynamicBrush.Key;
	NotificationIconMaterial = DynamicBrush.Value;

	// Structure
	ChildSlot
	.VAlign(VAlign_Fill)
	[
		SNew(SOverlay)

		// Notification box
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		.Padding(FMargin(0, 100))
		[
			SNew(SBox)
			.HeightOverride(Theme.NotificationDisplayHeight)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuGenericBorder)
				.ColorAndOpacity(this, &SNovaOverlay::GetColor)
				.BorderBackgroundColor(this, &SNovaOverlay::GetBackgroundColor)
				.Padding(FMargin(0, 1))
				[
					SNew(SBackgroundBlur)
					.BlurRadius(Theme.BlurRadius)
					.BlurStrength(Theme.BlurStrength)
					.bApplyAlphaToBlur(true)
					.Padding(0)
					[
						SNew(SBorder)
						.Padding(0)
						.BorderImage(&Theme.MainMenuGenericBackground)
						.ColorAndOpacity(this, &SNovaOverlay::GetColor)
						.BorderBackgroundColor(this, &SNovaOverlay::GetBackgroundColor)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SNovaOverlay::GetNotifyText)
							.TextStyle(&Theme.SubtitleFont)
							.ColorAndOpacity(this, &SNovaOverlay::GetTextColor)
						]
					]
				]
			]
		]

		// Notification icon
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		.Padding(FMargin(0, 75))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image(NotificationIconBrush.Get())
				.ColorAndOpacity(this, &SNovaOverlay::GetBackgroundColor)
			]
		]
	];

	// Defaults
	NotifyFadeDuration = ENovaUIConstants::FadeDurationShort;
	NotifyDisplayDuration = 5.0f;

	// Initialization
	CurrentNotifyAlpha = 0;
	CurrentNotifyFadeTime = 0;
	CurrentNotifyDisplayTime = NotifyDisplayDuration;
	SetVisibility(EVisibility::HitTestInvisible);
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaOverlay::Notify(FText Text, ENovaNotificationType Type)
{
	NLOG("SNovaOverlayMenu::Notify : %s", *Text.ToString());

	DesiredNotifyText = Text;
	DesiredNotifyType = Type;

	// Force update if we're past the original time
	if (CurrentNotifyDisplayTime > NotifyDisplayDuration)
	{
		CurrentNotifyText = FText();
	}
}

void SNovaOverlay::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update notification fade time
	if (CurrentNotifyDisplayTime > NotifyDisplayDuration || DesiredNotifyText.ToString() != CurrentNotifyText.ToString())
	{
		CurrentNotifyFadeTime -= DeltaTime;
	}
	else
	{
		CurrentNotifyFadeTime += DeltaTime;
	}
	CurrentNotifyFadeTime = FMath::Clamp(CurrentNotifyFadeTime, 0.0f, NotifyFadeDuration);
	CurrentNotifyAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentNotifyFadeTime / NotifyFadeDuration, ENovaUIConstants::EaseStandard);

	// Update notification text & icon when invisible
	if (CurrentNotifyFadeTime <= 0 && CurrentNotifyText.ToString() != DesiredNotifyText.ToString())
	{
		CurrentNotifyText = DesiredNotifyText;
		CurrentNotifyDisplayTime = 0;

		const FSlateBrush* TemplateNotificationBrush = nullptr;
		if (DesiredNotifyType == ENovaNotificationType::Info)
		{
			TemplateNotificationBrush = FNovaStyleSet::GetBrush("Notification/SB_NotificationIcon_Info");
		}
		else if (DesiredNotifyType == ENovaNotificationType::World)
		{
			TemplateNotificationBrush = FNovaStyleSet::GetBrush("Notification/SB_NotificationIcon_World");
		}
		else if (DesiredNotifyType == ENovaNotificationType::Saved)
		{
			TemplateNotificationBrush = FNovaStyleSet::GetBrush("Notification/SB_NotificationIcon_Saved");
		}
		else if (DesiredNotifyType == ENovaNotificationType::Error)
		{
			TemplateNotificationBrush = FNovaStyleSet::GetBrush("Notification/SB_NotificationIcon_Error");
		}

		if (TemplateNotificationBrush)
		{
			UTexture2D* IconTexture = Cast<UTexture2D>(TemplateNotificationBrush->GetResourceObject());
			NotificationIconMaterial->SetTextureParameterValue("MonitorTexture", IconTexture);
		}
	}
	else
	{
		CurrentNotifyDisplayTime += DeltaTime;
	}
}


/*----------------------------------------------------
	Content callbacks
----------------------------------------------------*/

FText SNovaOverlay::GetNotifyText() const
{
	return CurrentNotifyText;
}

FLinearColor SNovaOverlay::GetColor() const
{
	// Hack for proper smoothing of background blur
	return FLinearColor(1.0f, 1.0f, 1.0f, CurrentNotifyAlpha > 0.1f ? CurrentNotifyAlpha : 0.0f);
}

FSlateColor SNovaOverlay::GetBackgroundColor() const
{
	return GetColor();
}

FSlateColor SNovaOverlay::GetTextColor() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	FLinearColor Color = Theme.SubtitleFont.ColorAndOpacity.GetSpecifiedColor();
	Color.A *= CurrentNotifyAlpha;

	return Color;
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/


#undef LOCTEXT_NAMESPACE
