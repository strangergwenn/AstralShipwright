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
						.HAlign(HAlign_Center)
						.Padding(Theme.ContentPadding)
						[
							SAssignNew(TitleText, STextBlock)
							.TextStyle(&Theme.SubtitleFont)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(Theme.ContentPadding)
						[
							SAssignNew(InformationText, STextBlock)
							.TextStyle(&Theme.MainFont)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(Theme.ContentPadding)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							[
								SNovaNew(SNovaButton)
								.Action(FNovaPlayerInput::MenuConfirm)
								.Text(InArgs._ConfirmText.IsBound() ?
									InArgs._ConfirmText : InArgs._ConfirmText.IsSet() ?
									InArgs._ConfirmText.Get() : LOCTEXT("Confirm", "Confirm"))
								.HelpText(InArgs._ConfirmHelpText.IsBound()
									? InArgs._ConfirmHelpText : InArgs._ConfirmHelpText.IsSet()
									? InArgs._ConfirmHelpText.Get() : LOCTEXT("ConfirmHelp", "Confirm the current action"))
								.OnClicked(this, &SNovaModalPanel::OnConfirmPanel)
							]

							+ SHorizontalBox::Slot()
							[
								SNovaNew(SNovaButton)
								.Action(FNovaPlayerInput::MenuSecondary)
								.Text(InArgs._DismissText.IsBound() ?
									InArgs._DismissText : InArgs._DismissText.IsSet()
									? InArgs._DismissText.Get() : LOCTEXT("Ignore", "Ignore"))
								.HelpText(InArgs._DismissHelpText.IsBound()
									? InArgs._DismissHelpText : InArgs._DismissHelpText.IsSet()
									? InArgs._DismissHelpText.Get() : LOCTEXT("IgnoreHelp", "Ignore and close this message"))
								.OnClicked(this, &SNovaModalPanel::OnDismissPanel)
								.Visibility(this, &SNovaModalPanel::GetDismissVisibility)
							]

							+ SHorizontalBox::Slot()
							[
								SNovaNew(SNovaButton)
								.Action(FNovaPlayerInput::MenuCancel)
								.Text(InArgs._CancelText.IsBound() ?
									InArgs._CancelText : InArgs._CancelText.IsSet() ?
									InArgs._CancelText.Get() : LOCTEXT("Cancel", "Cancel"))
								.HelpText(InArgs._CancelHelpText.IsBound() ?
									InArgs._CancelHelpText : InArgs._CancelHelpText.IsSet() ?
									InArgs._CancelHelpText.Get() : LOCTEXT("CancelHelp", "Cancel the current action"))
								.OnClicked(this, &SNovaModalPanel::OnCancelPanel)
							]
						]

						+ SVerticalBox::Slot()
						[
							SAssignNew(ContentBox, SBox)
						]
					]
				]
			]
		]

		+ SVerticalBox::Slot()
	];
	// clang-format on

	SAssignNew(EmptyWidget, SBox);

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

bool SNovaModalPanel::Confirm()
{
	OnConfirmPanel();

	return true;
}

bool SNovaModalPanel::Cancel()
{
	OnCancelPanel();

	return true;
}

void SNovaModalPanel::Show(FText Title, FText Text, FSimpleDelegate NewOnConfirmed, FSimpleDelegate NewOnDismissed,
	FSimpleDelegate NewOnCancelled, TSharedPtr<SWidget> Content)
{
	NLOG("SNovaModalPanel::Show");

	ShouldShow  = true;
	OnConfirmed = NewOnConfirmed;
	OnDismissed = NewOnDismissed;
	OnCancelled = NewOnCancelled;

	TitleText->SetText(Title);
	InformationText->SetText(Text);

	if (Content.IsValid())
	{
		ContentBox->SetContent(Content.ToSharedRef());
	}
	else
	{
		ContentBox->SetContent(EmptyWidget.ToSharedRef());
	}

	if (Menu)
	{
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

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

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

	OnConfirmed.ExecuteIfBound();

	Hide();
}

void SNovaModalPanel::OnDismissPanel()
{
	NLOG("SNovaModalPanel::OnDismissPanel");

	OnDismissed.ExecuteIfBound();

	Hide();
}

void SNovaModalPanel::OnCancelPanel()
{
	NLOG("SNovaModalPanel::OnCancelPanel");

	OnCancelled.ExecuteIfBound();

	Hide();
}

#undef LOCTEXT_NAMESPACE
