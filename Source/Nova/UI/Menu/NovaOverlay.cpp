// Nova project - Gwennaël Arbona

#include "NovaOverlay.h"
#include "NovaMainMenu.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaModalPanel.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "SNovaOverlay"

/*----------------------------------------------------
    Notification widget
----------------------------------------------------*/

class SNovaNotification
	: public SNovaFadingWidget
	, public FGCObject
{
public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// Create material
		auto DynamicBrush        = FNovaStyleSet::GetDynamicBrush("Notification/SB_NotificationIcon_Base");
		NotificationIconBrush    = DynamicBrush.Key;
		NotificationIconMaterial = DynamicBrush.Value;

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
			.FadeDuration(ENovaUIConstants::FadeDurationShort)
			.DisplayDuration(5.0f)
		);

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
					.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
					.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
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
							.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
							.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SNovaNotification::GetNotifyText)
								.TextStyle(&Theme.SubtitleFont)
								.ColorAndOpacity(this, &SNovaNotification::GetTextColor)
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
					.ColorAndOpacity(this, &SNovaFadingWidget::GetSlateColor)
				]
			]
		];
		// clang-format on
	}

	virtual FString GetReferencerName() const override
	{
		return FString("SNovaNotification");
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(NotificationIconMaterial);
	}

	void Notify(const FText& Text, ENovaNotificationType Type)
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

	virtual bool IsDirty() const
	{
		return !CurrentNotifyText.EqualTo(DesiredNotifyText);
	}

	virtual void OnUpdate() override
	{
		CurrentNotifyText = DesiredNotifyText;

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

	FText GetNotifyText() const
	{
		return CurrentNotifyText;
	}

	FSlateColor GetTextColor() const
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
		return SNovaFadingWidget::GetTextColor(Theme.SubtitleFont);
	}

private:
	// Notification icon
	TSharedPtr<FSlateBrush>         NotificationIconBrush;
	class UMaterialInstanceDynamic* NotificationIconMaterial;

	// Notification state
	FText                 DesiredNotifyText;
	ENovaNotificationType DesiredNotifyType;
	FText                 CurrentNotifyText;
};

/*----------------------------------------------------
    Title card widget
----------------------------------------------------*/

class SNovaTitleCard : public SNovaFadingWidget
{
public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments()
			.FadeDuration(ENovaUIConstants::FadeDurationLong)
			.DisplayDuration(4.0f)
		);

		ChildSlot
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SNovaTitleCard::GetTitleText)
					.TextStyle(&Theme.TitleFont)
					.ColorAndOpacity(this, &SNovaTitleCard::GetTitleColor)
					.WrapTextAt(2000)
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SNovaTitleCard::GetSubtitleText)
					.TextStyle(&Theme.SubtitleFont)
					.ColorAndOpacity(this, &SNovaTitleCard::GetSubtitleColor)
					.WrapTextAt(2000)
					.Justification(ETextJustify::Center)
				]
			]
		];
		// clang-format on
	}

	virtual bool IsDirty() const
	{
		return !CurrentTitle.EqualTo(DesiredTitle) || !CurrentSubtitle.EqualTo(DesiredSubtitle);
	}

	virtual void OnUpdate() override
	{
		CurrentTitle    = DesiredTitle;
		CurrentSubtitle = DesiredSubtitle;
	}

	void ShowTitle(const FText& Title, const FText& Subtitle)
	{
		DesiredTitle    = Title;
		DesiredSubtitle = Subtitle;

		// Force update if we're past the original time
		if (CurrentDisplayTime > DisplayDuration)
		{
			CurrentTitle = FText();
		}
	}

	FText GetTitleText() const
	{
		return CurrentTitle.ToUpper();
	}

	FText GetSubtitleText() const
	{
		return CurrentSubtitle.ToUpper();
	}

	FSlateColor GetTitleColor() const
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
		return SNovaFadingWidget::GetTextColor(Theme.TitleFont);
	}

	FSlateColor GetSubtitleColor() const
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
		return SNovaFadingWidget::GetTextColor(Theme.SubtitleFont);
	}

private:
	// Title state
	FText DesiredTitle;
	FText DesiredSubtitle;
	FText CurrentTitle;
	FText CurrentSubtitle;
};

/*----------------------------------------------------
    Overlay implementation
----------------------------------------------------*/

void SNovaOverlay::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// clang-format off
	ChildSlot
	.VAlign(VAlign_Fill)
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(Notification, SNovaNotification)
		]

		+ SOverlay::Slot()
		[
			SAssignNew(TitleCard, SNovaTitleCard)
		]
	];
	// clang-format on

	// Initialization
	SetVisibility(EVisibility::HitTestInvisible);
}

void SNovaOverlay::Notify(const FText& Text, ENovaNotificationType Type)
{
	Notification->Notify(Text, Type);
}

void SNovaOverlay::ShowTitle(const FText& Title, const FText& Subtitle)
{
	TitleCard->ShowTitle(Title, Subtitle);
}

#undef LOCTEXT_NAMESPACE
