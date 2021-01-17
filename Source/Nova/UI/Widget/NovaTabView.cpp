// Nova project - Gwennaël Arbona

#include "NovaTabView.h"
#include "NovaMenu.h"

#include "Nova/UI/NovaUITypes.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"


/*----------------------------------------------------
	Tab view content widget
----------------------------------------------------*/

SNovaTabPanel::SNovaTabPanel()
	: SNovaNavigationPanel()
	, Blurred(false)
	, TabIndex(0)
	, CurrentVisible(false)
	, CurrentAlpha(0)
{}

void SNovaTabPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaNavigationPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Update opacity value
	if (TabIndex != ParentTabView->GetDesiredTabIndex())
	{
		CurrentAlpha -= (DeltaTime / ENovaUIConstants::FadeDurationShort);
	}
	else if (TabIndex == ParentTabView->GetCurrentTabIndex())
	{
		CurrentAlpha += (DeltaTime / ENovaUIConstants::FadeDurationShort);
	}
	CurrentAlpha = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);

	// Hide or show the menu based on current alpha
	if (!CurrentVisible && CurrentAlpha > 0)
	{
		Show();
	}
	else if (CurrentVisible && CurrentAlpha == 0)
	{
		Hide();
	}
}

void SNovaTabPanel::OnFocusChanged(TSharedPtr<class SNovaButton> FocusButton)
{
	if (MainContainer.IsValid())
	{
		if (IsButtonInsideWidget(FocusButton, MainContainer))
		{
			MainContainer->ScrollDescendantIntoView(FocusButton, true, EDescendantScrollDestination::Center);
		}
	}
}

void SNovaTabPanel::Initialize(int32 Index, bool IsBlurred, TSharedPtr<SNovaTabView> Parent)
{
	NCHECK(Parent.IsValid());

	Blurred = IsBlurred;
	TabIndex = Index;
	ParentTabView = Parent;
}

void SNovaTabPanel::Show()
{
	CurrentVisible = true;

	NCHECK(Menu);
	Menu->SetActiveNavigationPanel(this);
}

void SNovaTabPanel::Hide()
{
	CurrentVisible = false;
	
	NCHECK(Menu);
	Menu->ClearNavigationPanel();
}

bool SNovaTabPanel::IsHidden() const
{
	return (CurrentAlpha == 0);
}


/*----------------------------------------------------
	Construct
----------------------------------------------------*/

SNovaTabView::SNovaTabView()
	: DesiredTabIndex(0)
	, CurrentTabIndex(0)
	, CurrentBlurAlpha(0)
{}

void SNovaTabView::Construct(const FArguments& InArgs)
{
	// Data
	SlotInfo = InArgs.Slots;
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

	// Structure
	ChildSlot
	[
		SNew(SOverlay)

		// Fullscreen blur
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Padding(this, &SNovaTabView::GetBlurPadding)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNovaTabView::GetBlurRadius)
				.BlurStrength(this, &SNovaTabView::GetBlurStrength)
				.Padding(0)
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(0)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNovaTabView::GetHeaderBlurRadius)
				.BlurStrength(this, &SNovaTabView::GetHeaderBlurStrength)
				.Padding(0)
				[
					SAssignNew(HeaderContainer, SBorder)
					.BorderImage(&Theme.MainMenuBackground)
					.Padding(0)
					[
						SNew(SVerticalBox)
		
						// Header buttons
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(Theme.ContentPadding)
						[
							SNew(SHorizontalBox)
				
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								InArgs._LeftNavigation.Widget
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SAssignNew(Header, SHorizontalBox)
							]
				
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								InArgs._RightNavigation.Widget
							]

							+ SHorizontalBox::Slot()
				
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								InArgs._End.Widget
							]
						]

						// User-supplied header widget
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0)
						.HAlign(HAlign_Center)
						[
							InArgs._Header.Widget
						]
					]
				]
			]

			// Main view
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.ColorAndOpacity(this, &SNovaTabView::GetColor)
				.BorderBackgroundColor(this, &SNovaTabView::GetBackgroundColor)
				.Padding(0)
				[
					SAssignNew(Content, SWidgetSwitcher)
					.WidgetIndex(this, &SNovaTabView::GetCurrentTabIndex)
				]
			]
		]
	];

	// Slot contents
	int32 Index = 0;
	for (SNovaTabView::FSlot* TabSlot : InArgs.Slots)
	{
		// Add header entry
		Header->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Theme("TabButton")
			.Size("TabButtonSize")
			.Text(TabSlot->HeaderText)
			.HelpText(TabSlot->HeaderHelpText)
			.OnClicked(this, &SNovaTabView::SetTabIndex, Index)
			.Visibility(this, &SNovaTabView::GetTabVisibility, Index)
			.Enabled(this, &SNovaTabView::IsTabEnabled, Index)
			.Focusable(false)
		];
		
		// Add content
		SNovaTabPanel* TabPanel = static_cast<SNovaTabPanel*>(&TabSlot->GetWidget().Get());
		TabPanel->Initialize(Index, TabSlot->Blurred, MakeShareable(this));
		Content->AddSlot()
		[
			TabSlot->GetWidget()
		];

		Panels.Add(TabPanel);

		Index++;
	}
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SNovaTabView::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// If we lost the current tab, find another one
	if (!IsTabVisible(CurrentTabIndex) && !IsTabVisible(DesiredTabIndex))
	{
		for (int32 Index = 0; Index < SlotInfo.Num(); Index++)
		{
			int32 RelativeIndex = (Index / 2 + 1) * (Index % 2 != 0 ? 1 : -1);
			RelativeIndex = CurrentTabIndex + (RelativeIndex % SlotInfo.Num());

			if (RelativeIndex >= 0 && IsTabVisible(RelativeIndex))
			{
				NLOG("SNovaTabView::Tick : tab %d isn't visible, fallback to %d", CurrentTabIndex, RelativeIndex);
				SetTabIndex(RelativeIndex);
				break;
			}
		}
	}

	// Process widget change
	if (CurrentTabIndex != DesiredTabIndex)
	{
		if (GetCurrentTabContent()->IsHidden())
		{
			CurrentTabIndex = DesiredTabIndex;
		}
	}

	// Process blur
	CurrentBlurAlpha += (GetCurrentTabContent()->IsBlurred() ? 1 : -1) * (DeltaTime / ENovaUIConstants::FadeDurationMinimal);
	CurrentBlurAlpha = FMath::Clamp(CurrentBlurAlpha, 0.0f, 1.0f);
}

void SNovaTabView::Refresh()
{
	if (CurrentTabIndex == DesiredTabIndex)
	{
		GetCurrentTabContent()->Hide();
		GetCurrentTabContent()->Show();
	}
}

void SNovaTabView::ShowPreviousTab()
{
	for (int32 Index = CurrentTabIndex - 1; Index >= 0; Index--)
	{
		if (IsTabVisible(Index))
		{
			SetTabIndex(Index);
			break;
		}
	}
}

void SNovaTabView::ShowNextTab()
{
	for (int32 Index = CurrentTabIndex + 1; Index < SlotInfo.Num(); Index++)
	{
		if (IsTabVisible(Index))
		{
			SetTabIndex(Index);
			break;
		}
	}
}

void SNovaTabView::SetTabIndex(int32 Index)
{
	NLOG("SNovaTabView::SetTabIndex : %d, was %d", Index, CurrentTabIndex);

	if (Index >= 0 && Index < SlotInfo.Num() && Index != CurrentTabIndex)
	{
		DesiredTabIndex = Index;
	}
}

int32 SNovaTabView::GetCurrentTabIndex() const
{
	return CurrentTabIndex;
}

int32 SNovaTabView::GetDesiredTabIndex() const
{
	return DesiredTabIndex;
}

float SNovaTabView::GetCurrentTabAlpha() const
{
	return GetCurrentTabContent()->GetCurrentAlpha();
}

bool SNovaTabView::IsTabVisible(int32 Index) const
{
	return (!SlotInfo[Index]->IsVisible.IsBound() || SlotInfo[Index]->IsVisible.Get());
}

TSharedRef<SNovaTabPanel> SNovaTabView::GetCurrentTabContent() const
{
	return SharedThis(Panels[CurrentTabIndex]);
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

EVisibility SNovaTabView::GetTabVisibility(int32 Index) const
{
	if (SlotInfo[Index]->IsVisible.IsBound())
	{
		return SlotInfo[Index]->IsVisible.Get() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

bool SNovaTabView::IsTabEnabled(int32 Index) const
{
	return DesiredTabIndex != Index;
}

FLinearColor SNovaTabView::GetColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, GetCurrentTabAlpha());
}

FSlateColor SNovaTabView::GetBackgroundColor() const
{
	return FLinearColor(1.0f, 1.0f, 1.0f, CurrentBlurAlpha);
}

bool SNovaTabView::IsBlurSplit() const
{
	return !GetCurrentTabContent()->IsBlurred() || CurrentBlurAlpha < 1.0f;
}

FMargin SNovaTabView::GetBlurPadding() const
{
	float TopPadding = IsBlurSplit() ? HeaderContainer->GetCachedGeometry().Size.Y : 0;
	return FMargin(0, TopPadding, 0, 0);
}

TOptional<int32> SNovaTabView::GetBlurRadius() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentBlurAlpha, ENovaUIConstants::EaseStandard);
	return static_cast<int32>(Theme.BlurRadius * Alpha);
}

float SNovaTabView::GetBlurStrength() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	float Alpha = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentBlurAlpha, ENovaUIConstants::EaseStandard);
	return Theme.BlurStrength * Alpha;
}

TOptional<int32> SNovaTabView::GetHeaderBlurRadius() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	return static_cast<int32>(IsBlurSplit() ? Theme.BlurRadius : 0);
}

float SNovaTabView::GetHeaderBlurStrength() const
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	return IsBlurSplit() ? Theme.BlurStrength : 0;
}

