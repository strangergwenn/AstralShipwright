// Nova project - Gwennaël Arbona

#include "NovaModalPanel.h"
#include "NovaMenu.h"
#include "NovaButton.h"

#include "Nova/UI/NovaUITypes.h"
#include "Nova/UI/NovaUI.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaModalPanel"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaModalPanel::Construct(const FArguments& InArgs)
{
	// Setup
	const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()

		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(&ButtonTheme.Border)
			.ColorAndOpacity(this, &SNovaModalPanel::GetColor)
			.BorderBackgroundColor(this, &SNovaModalPanel::GetBackgroundColor)
			.Padding(FMargin(0, 1))
			.VAlign(VAlign_Center)
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
					.ColorAndOpacity(this, &SNovaModalPanel::GetColor)
					.BorderBackgroundColor(this, &SNovaModalPanel::GetBackgroundColor)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(&Theme.MainMenuGenericBorder)
							.Padding(Theme.ContentPadding)
							.HAlign(HAlign_Center)
							[
								SAssignNew(TitleText, STextBlock)
								.TextStyle(&Theme.HeadingFont)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.ContentPadding)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							[
								SAssignNew(InformationText, STextBlock)
								.TextStyle(&Theme.InfoFont)
								.WrapTextAt(750)
							]

							+ SVerticalBox::Slot()
							[
								SAssignNew(ContentBox, SBox)
								.MaxDesiredHeight(750)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.Padding(0)
							.BorderImage(&Theme.MainMenuGenericBorder)
							.Padding(Theme.ContentPadding)
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								[
									SNovaAssignNew(ConfirmButton, SNovaButton)
									.Text(FText())
									.Action(FNovaPlayerInput::MenuConfirm)
									.OnClicked(this, &SNovaModalPanel::OnConfirmPanel)
									.Visibility(this, &SNovaModalPanel::GetConfirmVisibility)
									.Enabled(this, &SNovaModalPanel::IsConfirmEnabled)
								]

								+ SHorizontalBox::Slot()
								[
									SNovaAssignNew(DismissButton, SNovaButton)
									.Text(FText())
									.Action(FNovaPlayerInput::MenuSecondary)
									.OnClicked(this, &SNovaModalPanel::OnDismissPanel)
									.Visibility(this, &SNovaModalPanel::GetDismissVisibility)
								]

								+ SHorizontalBox::Slot()
								[
									SNovaAssignNew(CancelButton, SNovaButton)
									.Text(FText())
									.Action(FNovaPlayerInput::MenuCancel)
									.OnClicked(this, &SNovaModalPanel::OnCancelPanel)
								]
							]
						]
					]
				]
			]
		]

		+ SVerticalBox::Slot()
	];
	// clang-format on

	SAssignNew(InternalWidget, SBox);

	// Defaults
	FadeDuration = ENovaUIConstants::FadeDurationShort;

	// Initialization
	ShouldShow      = false;
	CurrentAlpha    = 0;
	CurrentFadeTime = 0;
	SetVisibility(EVisibility::HitTestInvisible);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaModalPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaNavigationPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update alpha
	if (ShouldShow)
	{
		CurrentFadeTime += DeltaTime;
	}
	else
	{
		CurrentFadeTime -= DeltaTime;
	}
	CurrentFadeTime = FMath::Clamp(CurrentFadeTime, 0.0f, FadeDuration);
	CurrentAlpha    = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentFadeTime / FadeDuration, ENovaUIConstants::EaseStandard);

	// Update visibility
	if (CurrentAlpha > 0)
	{
		SetVisibility(EVisibility::Visible);
	}
	else
	{
		SetVisibility(EVisibility::HitTestInvisible);
	}
}

void SNovaModalPanel::Show(FText Title, FText Text, FSimpleDelegate NewOnConfirmed, FSimpleDelegate NewOnDismissed,
	FSimpleDelegate NewOnCancelled, TSharedPtr<SWidget> Content)
{
	NLOG("SNovaModalPanel::Show");

	ShouldShow  = true;
	OnConfirmed = NewOnConfirmed;
	OnDismissed = NewOnDismissed;
	OnCancelled = NewOnCancelled;

	UpdateButtons();
	TitleText->SetText(Title);
	InformationText->SetText(Text);
	InformationText->SetVisibility(Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);

	if (Content.IsValid())
	{
		ContentBox->SetContent(Content.ToSharedRef());
	}
	else
	{
		ContentBox->SetContent(InternalWidget.ToSharedRef());
	}

	if (Menu)
	{
		ResetNavigation();
		Menu->SetModalNavigationPanel(this);
	}
}

void SNovaModalPanel::Hide()
{
	NLOG("SNovaModalPanel::Hide");

	ShouldShow = false;

	if (Menu && Menu->IsActiveNavigationPanel(this))
	{
		Menu->ClearModalNavigationPanel();
	}
}

void SNovaModalPanel::UpdateButtons()
{
	ConfirmButton->SetText(LOCTEXT("Confirm", "Confirm"));
	ConfirmButton->SetHelpText(LOCTEXT("ConfirmHelp", "Confirm the current action"));
	DismissButton->SetText(LOCTEXT("Ignore", "Ignore"));
	DismissButton->SetHelpText(LOCTEXT("IgnoreHelp", "Ignore and close this message"));
	CancelButton->SetText(LOCTEXT("Cancel", "Cancel"));
	CancelButton->SetHelpText(LOCTEXT("CancelHelp", "Cancel the current action"));
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

EVisibility SNovaModalPanel::GetConfirmVisibility() const
{
	return OnConfirmed.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SNovaModalPanel::IsConfirmEnabled() const
{
	return true;
}

EVisibility SNovaModalPanel::GetDismissVisibility() const
{
	return OnDismissed.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
}

FLinearColor SNovaModalPanel::GetColor() const
{
	// Hack for proper smoothing of background blur
	return FLinearColor(1.0f, 1.0f, 1.0f, CurrentAlpha > 0.1f ? CurrentAlpha : 0.0f);
}

FSlateColor SNovaModalPanel::GetBackgroundColor() const
{
	return GetColor();
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaModalPanel::OnConfirmPanel()
{
	NLOG("SNovaModalPanel::OnConfirmPanel");

	Hide();

	OnConfirmed.ExecuteIfBound();
}

void SNovaModalPanel::OnDismissPanel()
{
	NLOG("SNovaModalPanel::OnDismissPanel");

	Hide();

	OnDismissed.ExecuteIfBound();
}

void SNovaModalPanel::OnCancelPanel()
{
	NLOG("SNovaModalPanel::OnCancelPanel");

	Hide();

	OnCancelled.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
