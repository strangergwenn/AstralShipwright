// Nova project - Gwennaël Arbona

#include "NovaButton.h"
#include "NovaMenu.h"
#include "NovaKeyLabel.h"

#include "Nova/System/NovaMenuManager.h"
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
	Text                  = InArgs._Text;
	HelpText              = InArgs._HelpText;
	Action                = InArgs._Action;
	ThemeName             = InArgs._Theme;
	SizeName              = InArgs._Size;
	Icon                  = InArgs._Icon;
	ButtonEnabled         = InArgs._Enabled;
	ButtonFocusable       = (InArgs._Action.Get() != NAME_None && !InArgs._ActionFocusable) ? false : InArgs._Focusable;
	ButtonActionFocusable = InArgs._ActionFocusable;
	BorderRotation        = InArgs._BorderRotation;
	IsToggle              = InArgs._Toggle;
	OnFocused             = InArgs._OnFocused;
	OnClicked             = InArgs._OnClicked;
	OnDoubleClicked       = InArgs._OnDoubleClicked;
	UserSizeCallback      = InArgs._UserSizeCallback;

	// Setup
	Focused                       = false;
	Hovered                       = false;
	Active                        = false;
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

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
				.WidthOverride(this, &SNovaButton::GetVisualWidth)
				.HeightOverride(this, &SNovaButton::GetVisualHeight)
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
				.WidthOverride(this, &SNovaButton::GetLayoutWidth)
				.HeightOverride(this, &SNovaButton::GetLayoutHeight)
				.Padding(Theme.HoverAnimationPadding)
				[
					SAssignNew(InnerContainer, SBorder)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.BorderImage(new FSlateNoResource)
					[
						InArgs._Content.Widget
					]
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
	
	// Get text & icon data
	const bool HasIcon = IsToggle || InArgs._Icon.IsSet() || InArgs._Icon.IsBound();
	const bool HasAction = InArgs._Action.IsSet() || InArgs._Action.IsBound();
	const bool IsFullWidthText = !HasIcon && !HasAction && !Theme.Centered;
	WrapSize = Size.Width - Theme.WrapMargin / 2;
	if (HasIcon)
	{
		WrapSize -= Theme.WrapMargin;
	}
	if (HasAction)
	{
		WrapSize -= Theme.WrapMargin;
	}

	// Construct the default internal layout
	if (InArgs._Text.IsSet() || InArgs._Icon.IsSet())
	{
		// Create the box
		TSharedPtr<SHorizontalBox> Box;
		SAssignNew(Box, SHorizontalBox);
		
		// Create the icon/action box
		TSharedPtr<SOverlay> IconOverlay;
		if (HasIcon || HasAction)
		{
			Box->AddSlot()
			.AutoWidth()
			[
				SNew(SBox)
				.Padding(Theme.IconPadding)
				[
					SAssignNew(IconOverlay, SOverlay)
				]
			];
		}
		
		// Centering
		if (Theme.Centered)
		{
			Box->AddSlot();
		}

		// Create the text block
		Box->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(IsFullWidthText ? Theme.IconPadding : Theme.Centered ? FMargin(-1, 0, 0, 0) : FMargin())
		[
			SAssignNew(TextBlock, STextBlock)
			.TextStyle(&Theme.Font)
			.Font(this, &SNovaButton::GetFont)
			.Text(InArgs._Text)
			.WrapTextAt(WrapSize)
		];
		
		// Centering
		if (Theme.Centered)
		{
			Box->AddSlot();
		}

		// Add icon if set or bound
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
		if (HasAction)
		{
			IconOverlay->AddSlot()
			.VAlign(VAlign_Center)
			.Padding(HasIcon ? FMargin(25, 0, 0, 0) : FMargin(0))
			[
				SNew(SNovaKeyLabel)
				.Key(this, &SNovaButton::GetActionKey)
			];
		}

		InnerContainer->SetContent(Box.ToSharedRef());
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

	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);

	bIsFocusable = ButtonFocusable.Get();

	float TargetColorAnim    = 0;
	float TargetSizeAnim     = 0;
	float TargetDisabledAnim = 0;

	// Decide how animations should evolve
	if (IsButtonEnabled())
	{
		if (IsFocused())
		{
			TargetColorAnim = 1.0f;
			TargetSizeAnim  = 1.0f;
		}
		if (Hovered)
		{
			TargetSizeAnim = 0.75f;
		}
	}
	else
	{
		TargetDisabledAnim = 1.0f;
	}
	if (State.CurrentTimeSinceClicked < Theme.AnimationDuration)
	{
		TargetSizeAnim = 0.5f;
	}

	// Update function to control an animation state based on a target
	auto UpdateAlpha = [&](float& CurrentValue, float TargetValue)
	{
		float Difference = FMath::Clamp(TargetValue - CurrentValue, -1.0f, 1.0f);
		if (Difference != 0)
		{
			float PreviousValue = CurrentValue;
			CurrentValue += (CurrentValue < TargetValue ? 1 : -1) * (DeltaTime / Theme.AnimationDuration);
			CurrentValue = FMath::Clamp(CurrentValue, PreviousValue - FMath::Abs(Difference), PreviousValue + FMath::Abs(Difference));
		}
	};

	// Update the timings
	UpdateAlpha(State.CurrentColorAnimationAlpha, TargetColorAnim);
	UpdateAlpha(State.CurrentSizeAnimationAlpha, TargetSizeAnim);
	UpdateAlpha(State.CurrentDisabledAnimationAlpha, TargetDisabledAnim);

	// Process size directly because it doesn't have any added processing
	if (UserSizeCallback.IsBound())
	{
		State.CurrentUserSizeAnimationAlpha = UserSizeCallback.Execute();
	}
	else
	{
		State.CurrentUserSizeAnimationAlpha = 0.0f;
	}

	State.CurrentTimeSinceClicked += DeltaTime;
}

void SNovaButton::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SButton::OnMouseEnter(MyGeometry, MouseEvent);

	UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);
	if (!MenuManager->IsUsingGamepad())
	{
		Hovered = true;
		MenuManager->ShowTooltip(this, HelpText.Get());
	}
}

void SNovaButton::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SButton::OnMouseLeave(MouseEvent);

	UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);
	if (!MenuManager->IsUsingGamepad())
	{
		Hovered = false;
		MenuManager->HideTooltip(this);
	}
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

FKey SNovaButton::GetActionKey() const
{
	return UNovaMenuManager::Get()->GetFirstActionKey(Action.Get());
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

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

	UNovaMenuManager* MenuManager = UNovaMenuManager::Get();
	NCHECK(MenuManager);
	FLinearColor MainColor = MenuManager->GetInterfaceColor();

	return FMath::InterpEaseInOut(MainColor, Theme.DisabledColor, State.CurrentDisabledAnimationAlpha, ENovaUIConstants::EaseLight);
}

FOptionalSize SNovaButton::GetVisualWidth() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	float DisabledOffset =
		FMath::InterpEaseInOut(0.0f, Size.DisabledAnimationSize.X, State.CurrentDisabledAnimationAlpha, ENovaUIConstants::EaseLight);
	float FocusedOffset = State.CurrentUserSizeAnimationAlpha * Size.UserAnimationSize.X;
	float Width         = Size.Width + DisabledOffset + FocusedOffset;

	float Offset = FMath::InterpEaseInOut(Theme.HoverAnimationPadding.Left + Theme.HoverAnimationPadding.Right, 0.0f,
		State.CurrentSizeAnimationAlpha, ENovaUIConstants::EaseLight);

	float RotationFactor = FMath::Abs(FMath::Cos(FMath::DegreesToRadians(BorderRotation)));

	return (RotationFactor * Width) - 2 - Offset;
}

FOptionalSize SNovaButton::GetVisualHeight() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	float DisabledOffset =
		FMath::InterpEaseInOut(0.0f, Size.DisabledAnimationSize.Y, State.CurrentDisabledAnimationAlpha, ENovaUIConstants::EaseLight);
	float FocusedOffset = State.CurrentUserSizeAnimationAlpha * Size.UserAnimationSize.Y;
	float Height        = Size.Height + DisabledOffset + FocusedOffset;

	float Offset = FMath::InterpEaseInOut(Theme.HoverAnimationPadding.Top + Theme.HoverAnimationPadding.Bottom, 0.0f,
		State.CurrentSizeAnimationAlpha, ENovaUIConstants::EaseLight);

	float RotationFactor = FMath::Abs(FMath::Cos(FMath::DegreesToRadians(BorderRotation)));

	return (RotationFactor * Height) - 2 - Offset;
}

FOptionalSize SNovaButton::GetLayoutWidth() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	float DisabledOffset =
		FMath::InterpEaseInOut(0.0f, Size.DisabledAnimationSize.X, State.CurrentDisabledAnimationAlpha, ENovaUIConstants::EaseLight);
	float FocusedOffset = State.CurrentUserSizeAnimationAlpha * Size.UserAnimationSize.X;

	return Size.Width + DisabledOffset + FocusedOffset;
}

FOptionalSize SNovaButton::GetLayoutHeight() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	const FNovaButtonSize&  Size  = FNovaStyleSet::GetButtonSize(SizeName);

	float DisabledOffset =
		FMath::InterpEaseInOut(0.0f, Size.DisabledAnimationSize.Y, State.CurrentDisabledAnimationAlpha, ENovaUIConstants::EaseLight);
	float FocusedOffset = State.CurrentUserSizeAnimationAlpha * Size.UserAnimationSize.Y;

	return Size.Height + DisabledOffset + FocusedOffset;
}

FSlateFontInfo SNovaButton::GetFont() const
{
	const FNovaButtonTheme& Theme = FNovaStyleSet::GetButtonTheme(ThemeName);
	FSlateFontInfo          Font  = Theme.Font.Font;

	if (Text.IsSet() || Text.IsBound())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		float TextSize = FontMeasureService->Measure(Text.Get(), Theme.Font.Font).X;
		if (TextSize > WrapSize)
		{
			float Ratio = FMath::Max(WrapSize / TextSize, 0.75f);
			Font.Size *= Ratio;
		}
	}

	return Font;
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
	if (OnDoubleClicked.IsBound())
	{
		OnDoubleClicked.Execute();
	}
	else
	{
		OnButtonClicked();
	}

	return FReply::Handled();
}
