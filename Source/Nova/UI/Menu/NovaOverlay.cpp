// Nova project - Gwennaël Arbona

#include "NovaOverlay.h"

#include "Nova/UI/Component/NovaEventDisplay.h"
#include "Nova/UI/Component/NovaNotification.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"

#include "Nova/System/NovaMenuManager.h"
#include "Nova/Nova.h"

#define LOCTEXT_NAMESPACE "SNovaOverlay"

/*----------------------------------------------------
    Title card widget
----------------------------------------------------*/

class SNovaTitleCard : public SNovaFadingWidget<>
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
				SNew(SBorder)
				.Padding(Theme.ContentPadding)
				.BorderImage(new FSlateNoResource)
				.ColorAndOpacity(this, &SNovaFadingWidget::GetLinearColor)
				.Padding(0)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(this, &SNovaTitleCard::GetTitleText)
						.TextStyle(&Theme.TitleFont)
						.WrapTextAt(2000)
						.Justification(ETextJustify::Center)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(this, &SNovaTitleCard::GetSubtitleText)
						.TextStyle(&Theme.HeadingFont)
						.WrapTextAt(2000)
						.Justification(ETextJustify::Center)
					]
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
			SNew(SNovaEventDisplay)
			.MenuManager(InArgs._MenuManager)
		]

		+ SOverlay::Slot()
		[
			SAssignNew(FastForward, SBorder)
			.BorderImage(FNovaStyleSet::GetBrush("Common/SB_Black"))
		]

		+ SOverlay::Slot()
		[
			SAssignNew(TitleCard, SNovaTitleCard)
		]

		+ SOverlay::Slot()
		[
			SAssignNew(Notification, SNovaNotification)
			.MenuManager(InArgs._MenuManager)
		]
	];
	// clang-format on

	// Initialization
	SetVisibility(EVisibility::HitTestInvisible);
	FastForward->SetVisibility(EVisibility::Collapsed);
}

void SNovaOverlay::Notify(const FText& Text, ENovaNotificationType Type)
{
	Notification->Notify(Text, Type);
}

void SNovaOverlay::ShowTitle(const FText& Title, const FText& Subtitle)
{
	TitleCard->ShowTitle(Title, Subtitle);
}

void SNovaOverlay::StartFastForward()
{
	FastForward->SetVisibility(EVisibility::Visible);
}

void SNovaOverlay::StopFastForward()
{
	FastForward->SetVisibility(EVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
