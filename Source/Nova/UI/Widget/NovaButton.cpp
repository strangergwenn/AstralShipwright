// Nova project - Gwennaël Arbona

#include "NovaButton.h"
#include "NovaMenu.h"
#include "NovaKeyLabel.h"

#include "Nova/Player/NovaMenuManager.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Nova.h"

#include "Fonts/SlateFontInfo.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

/*----------------------------------------------------
    Construct
----------------------------------------------------*/

void SNovaButton::Construct(const FArguments& InArgs)
{
	// Arguments
	Text            = InArgs._Text;
	HelpText        = InArgs._HelpText;
	Action          = InArgs._Action;
	ThemeName       = InArgs._Theme;
	SizeName        = InArgs._Size;
	Icon            = InArgs._Icon;
	ButtonEnabled   = InArgs._Enabled;
	ButtonFocusable = InArgs._Focusable;
	BorderRotation  = InArgs._BorderRotation;
	IsToggle        = InArgs._Toggle;
	OnFocused       = InArgs._OnFocused;
	OnClicked       = InArgs._OnClicked;
	OnDoubleClicked = InArgs._OnDoubleClicked;

	// Setup
	Focused                       = false;
	Hovered                       = false;
	Active                        = false;
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	// Settings
	AnimationDuration = 0.2f;

	// Parent constructor
	SButton::Construct(SButton::FArguments()
						   .ButtonStyle(FNovaStyleSet::GetStyle(), "Nova.Button")
						   .ButtonColorAndOpacity(FLinearColor(0, 0, 0, 0))
						   .ContentPadding(0)
						   .OnClicked(this, &SNovaButton::OnButtonClicked));

	// clang-format off
	ChildSlot
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[
		SNew(SVerticalBox)

		// User-supplied header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			InArgs._Header.Widget
		]
	
		// Main button body
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(this, &SNovaButton::GetWidth)
				.HeightOverride(this, &SNovaButton::GetHeight)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.Border)
					.Padding(FMargin(1))
					.RenderTransform(this, &SNovaButton::GetBorderRenderTransform)
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
					[
						SNew(SImage)
						.Image(&Theme.Background)
						.ColorAndOpacity(this, &SNovaButton::GetBackgroundBrushColor)
					]
				]
			]

			+ SOverlay::Slot()
			[
				SNew(SBox)
				.WidthOverride(Size.Width)
				.HeightOverride(Size.Height)
				.Padding(Theme.AnimationPadding)
				[
					SAssignNew(InnerContainer, SBorder)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.BorderImage(new FSlateNoResource)
				]
			]
		]

		// User-supplied footer
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			InArgs._Footer.Widget
		]
	];
	InnerContainer->SetClipping(EWidgetClipping::ClipToBoundsAlways);

	// Construct the default internal layout
	if (InArgs._Text.IsSet() || InArgs._Icon.IsSet())
	{
		TSharedPtr<SOverlay> IconOverlay;

		InnerContainer->SetContent(

			SNew(SHorizontalBox)

			// Icon / action icon
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(Theme.IconPadding)
			[
				SAssignNew(IconOverlay, SOverlay)
			]

			// Text
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(TextBlock, STextBlock)
				.TextStyle(&Theme.MainFont)
				.Font(this, &SNovaButton::GetFont)
				.Text(InArgs._Text)
			]);

		// Add icon if set or bound
		bool HasIcon = IsToggle || InArgs._Icon.IsSet() || InArgs._Icon.IsBound();
		if (HasIcon)
		{
			IconOverlay->AddSlot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &SNovaButton::GetIconBrush)
			];
		}

		// Add the action key if any
		if (InArgs._Action.IsSet() || InArgs._Action.IsBound())
		{
			IconOverlay->AddSlot()
			.VAlign(VAlign_Center)
			.Padding(HasIcon ? FMargin(25, 0, 0, 0) : FMargin(0))
			[
				SNew(SNovaKeyLabel)
				.Key(this, &SNovaButton::GetActionKey)
			];
		}
	}
	else
	{
		InnerContainer->SetHAlign(HAlign_Fill);
		InnerContainer->SetVAlign(VAlign_Fill);
	}
	// clang-format on

	// Set opacity
	ColorAndOpacity.BindRaw(this, &SNovaButton::GetMainColor);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaButton::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SButton::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	bIsFocusable = ButtonFocusable.Get();

	// Decide how animations should evolve
	float TargetColorAnim    = 0;
	float TargetSizeAnim     = 0;
	float TargetDisabledAnim = 0;
	if (IsButtonEnabled())
	{
		if (IsFocused())
		{
			TargetColorAnim = 1.0f;
			TargetSizeAnim  = 0.5f;
		}
		if (IsHovered())
		{
			TargetSizeAnim = 0.75f;
		}
	}
	else
	{
		TargetDisabledAnim = 1.0f;
	}
	if (State.CurrentTimeSinceClicked < AnimationDuration)
	{
		TargetSizeAnim = 1.0f;
	}

	// Update function to control an animation state based on a target
	auto UpdateAlpha = [&](float& CurrentValue, float TargetValue)
	{
		float Difference = FMath::Clamp(TargetValue - CurrentValue, -1.0f, 1.0f);
		if (Difference != 0)
		{
			float PreviousValue = CurrentValue;
			CurrentValue += (CurrentValue < TargetValue ? 1 : -1) * (DeltaTime / AnimationDuration);
			CurrentValue = FMath::Clamp(CurrentValue, PreviousValue - FMath::Abs(Difference), PreviousValue + FMath::Abs(Difference));
		}
	};

	// Update the timings
	UpdateAlpha(State.CurrentColorAnimationAlpha, TargetColorAnim);
	UpdateAlpha(State.CurrentSizeAnimationAlpha, TargetSizeAnim);
	UpdateAlpha(State.CurrentDisabledAnimationAlpha, TargetDisabledAnim);
	State.CurrentTimeSinceClicked += DeltaTime;
}

void SNovaButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SButton::OnMouseEnter(MyGeometry, MouseEvent);

	Hovered = true;

	UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);

	MenuManager->ShowTooltip(this, HelpText.Get());
}

void SNovaButton::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SButton::OnMouseLeave(MouseEvent);

	Hovered = false;

	UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);

	MenuManager->HideTooltip(this);
}

void SNovaButton::SetFocused(bool FocusedState)
{
	UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);

	Focused = FocusedState;

	if (Focused)
	{
		NCHECK(SupportsKeyboardFocus());
		MenuManager->ShowTooltip(this, HelpText.Get());
		OnFocused.ExecuteIfBound();
	}
	else
	{
		MenuManager->HideTooltip(this);
	}
}

bool SNovaButton::IsFocused() const
{
	return Focused;
}

bool SNovaButton::IsButtonEnabled() const
{
	return ButtonEnabled.Get(true);
}

void SNovaButton::SetActive(bool ActiveState)
{
	Active = ActiveState;
}

bool SNovaButton::IsActive() const
{
	return Active;
}

void SNovaButton::SetText(FText NewText)
{
	if (TextBlock.IsValid())
	{
		Text.Set(NewText);
		TextBlock->SetText(Text);
	}
}

void SNovaButton::SetHelpText(FText NewText)
{
	HelpText.Set(NewText);
}

FText SNovaButton::GetHelpText()
{
	return HelpText.Get();
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

FKey SNovaButton::GetActionKey() const
{
	return UNovaMenuManager::Get()->GetFirstActionKey(Action.Get());
}

const FSlateBrush* SNovaButton::GetIconBrush() const
{
	if (Icon.IsSet())
	{
		return Icon.Get();
	}
	else if (IsToggle)
	{
		return (Active ? FNovaStyleSet::GetBrush("Icon/SB_On") : FNovaStyleSet::GetBrush("Icon/SB_Off"));
	}
	else
	{
		return NULL;
	}
}

FSlateColor SNovaButton::GetBackgroundBrushColor() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);

	FLinearColor Color = FLinearColor::White;
	Color.A            = FMath::InterpEaseInOut(0.0f, 1.0f, State.CurrentColorAnimationAlpha, ENovaUIConstants::EaseStandard);

	return Color;
}

FLinearColor SNovaButton::GetMainColor() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);

	return FMath::InterpEaseInOut(
		FLinearColor::White, Theme.DisabledColor, State.CurrentDisabledAnimationAlpha, ENovaUIConstants::EaseLight);
}

FOptionalSize SNovaButton::GetWidth() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	float Offset = FMath::InterpEaseInOut(
		Theme.AnimationPadding.Left + Theme.AnimationPadding.Right, 0.0f, State.CurrentSizeAnimationAlpha, ENovaUIConstants::EaseLight);

	float RotationFactor = FMath::Abs(FMath::Cos(FMath::DegreesToRadians(BorderRotation)));

	return RotationFactor * Size.Width - 2 - Offset;
}

FOptionalSize SNovaButton::GetHeight() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	float Offset = FMath::InterpEaseInOut(
		Theme.AnimationPadding.Top + Theme.AnimationPadding.Bottom, 0.0f, State.CurrentSizeAnimationAlpha, ENovaUIConstants::EaseLight);

	float RotationFactor = FMath::Abs(FMath::Cos(FMath::DegreesToRadians(BorderRotation)));

	return RotationFactor * Size.Height - 2 - Offset;
}

FSlateFontInfo SNovaButton::GetFont() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);

	if (Text.IsSet() || Text.IsBound())
	{
		float TextLength  = Text.Get().ToString().Len();
		float ButtonWidth = GetDesiredSize().X;

		if (TextLength > 0.09 * ButtonWidth)
		{
			return Theme.SmallFont.Font;
		}
	}

	return Theme.MainFont.Font;
}

TOptional<FSlateRenderTransform> SNovaButton::GetBorderRenderTransform() const
{
	return FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(BorderRotation)));
}

FReply SNovaButton::OnButtonClicked()
{
	if (IsButtonEnabled())
	{
		if (IsToggle)
		{
			Active = !Active;
		}

		State.CurrentTimeSinceClicked = 0;

		UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
		NCHECK(MenuManager);
		if (SupportsKeyboardFocus())
		{
			MenuManager->GetMenu()->SetFocusedButton(SharedThis(this), false);
		}

		if (OnClicked.IsBound() == true)
		{
			OnClicked.Execute();
		}
	}

	return FReply::Handled();
}

FReply SNovaButton::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnDoubleClicked.ExecuteIfBound();

	return FReply::Handled();
}
