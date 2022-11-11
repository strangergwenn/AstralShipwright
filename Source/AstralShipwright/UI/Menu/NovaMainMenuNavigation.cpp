// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuNavigation.h"

#include "NovaMainMenu.h"

#include "Game/NovaArea.h"
#include "Game/NovaAISpacecraft.h"
#include "Game/NovaAsteroid.h"
#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Player/NovaPlayerController.h"
#include "UI/Widgets/NovaTrajectoryCalculator.h"
#include "UI/Widgets/NovaTradableAssetItem.h"
#include "Nova.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"
#include "Neutron/UI/Widgets/NeutronKeyLabel.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"
#include "Neutron/UI/Widgets/NeutronSlider.h"
#include "Neutron/UI/Widgets/NeutronTable.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuNavigation"

// Settings
static constexpr int32 StackPanelWidth = 500;

/*----------------------------------------------------
    Hover details stack
----------------------------------------------------*/

/** Fading text widget with a background, inheriting SNovaFadingWidget for simplicity */
class SNovaHoverStackEntry : public SNeutronText
{
public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		SNeutronText::Construct(SNeutronText::FArguments()
									.TextStyle(&Theme.MainFont)
									.WrapTextAt(StackPanelWidth - Theme.ContentPadding.Left - Theme.ContentPadding.Right - 24));

		// clang-format off
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBackground)
			.Padding(Theme.ContentPadding)
			.BorderBackgroundColor(this, &SNeutronFadingWidget::GetSlateColor)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image_Lambda([=]()
					{
						return FNeutronStyleSet::GetBrush("Icon/SB_Thumb");
					})
					.ColorAndOpacity_Lambda([=]()
					{
						FLinearColor NewColor = FLinearColor::White;
						NewColor.A *= CurrentAlpha * (IsSelected ? 1.0f : 0.0f);
						return NewColor;
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image_Lambda([=]()
					{
						return Brush;
					})
					.ColorAndOpacity_Lambda([=]()
					{
						FLinearColor NewColor = Color;
						NewColor.A *= CurrentAlpha;
						return NewColor;
					})
				]

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					TextBlock.ToSharedRef()
				]
			]
		];
		// clang-format on
	}

	void SetVisualEffects(const FSlateBrush* NewBrush, const FLinearColor NewColor, bool Selected)
	{
		Brush      = NewBrush;
		Color      = NewColor;
		IsSelected = Selected;
	}

protected:

	const FSlateBrush* Brush;
	FLinearColor       Color;
	bool               IsSelected;
};

/** Text stack with fading lines */
class SNovaHoverStack : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SNovaHoverStack)
	{}

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

		// clang-format off
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(StackPanelWidth)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBackground)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(Theme.ContentPadding)
						[
							SNew(SNeutronKeyLabel)
							.Action(FNeutronPlayerInput::MenuPrevious)
						]

						+ SHorizontalBox::Slot()
						.Padding(Theme.ContentPadding)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(&Theme.InfoFont)
							.Text(LOCTEXT("SelectedObjects", "Selected objects"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(Theme.ContentPadding)
						[
							SNew(SNeutronKeyLabel)
							.Action(FNeutronPlayerInput::MenuNext)
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(Container, SVerticalBox)
				]
			]
		];
		// clang-format on
	}

public:

	void Update(
		const ANovaGameState* GameState, TSharedPtr<SNovaOrbitalMap> OrbitalMap, TArray<FNovaOrbitalObject> ObjectList, int32 SelectedIndex)
	{
		if (IsValid(GameState))
		{
			const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

			// Text builder
			auto GetText = [GameState](const FNovaOrbitalObject& Object)
			{
				if (Object.Area.IsValid())
				{
					return Object.Area->Name;
				}
				else if (Object.AsteroidIdentifier != FGuid())
				{
					return FText::FromString(Object.AsteroidIdentifier.ToString(EGuidFormats::Short).ToUpper());
				}
				else if (Object.SpacecraftIdentifier != FGuid())
				{
					const FNovaSpacecraft* Spacecraft = GameState->GetSpacecraft(Object.SpacecraftIdentifier);
					NCHECK(Spacecraft);
					return Spacecraft ? Spacecraft->GetName() : FText();
				}
				else if (Object.Maneuver.IsValid())
				{
					FNumberFormattingOptions NumberOptions;
					NumberOptions.SetMaximumFractionalDigits(1);

					FNovaTime TimeLeftBeforeManeuver = Object.Maneuver->Time - GameState->GetCurrentTime();
					FText     ManeuverTextFormat     = TimeLeftBeforeManeuver > 0
					                                     ? LOCTEXT("ManeuverFormatValid", "{duration} burn for a {deltav} m/s maneuver in {time}")
					                                     : LOCTEXT("ManeuverFormatExpired", "{duration} burn for a {deltav} m/s maneuver");

					return FText::FormatNamed(ManeuverTextFormat, TEXT("time"), GetDurationText(TimeLeftBeforeManeuver, 2),
						TEXT("duration"), GetDurationText(Object.Maneuver->Duration), TEXT("deltav"),
						FText::AsNumber(Object.Maneuver->DeltaV, &NumberOptions));
				}
				else
				{
					return FText();
				}
			};

			// Color builder
			auto GetColor = [OrbitalMap, Theme](const FNovaOrbitalObject& Object)
			{
				if (OrbitalMap->GetPreviewTrajectory().IsValid() && Object.Maneuver.IsValid())
				{
					return Theme.ContrastingColor;
				}
				else if (Object.Maneuver.IsValid())
				{
					return Theme.PositiveColor;
				}
				else if (Object.SpacecraftIdentifier != FGuid())
				{
					return Theme.PositiveColor;
				}
				else
				{
					return FLinearColor::White;
				}
			};

			// Grow the text item list if too small
			int32 TextItemsToAdd = ObjectList.Num() - TextItemList.Num();
			if (TextItemsToAdd > 0)
			{
				for (int32 Index = 0; Index < TextItemsToAdd; Index++)
				{
					TSharedPtr<SNovaHoverStackEntry> NewItem = SNew(SNovaHoverStackEntry);

					Container->AddSlot().AutoHeight()[NewItem.ToSharedRef()];
					TextItemList.Add(NewItem);
				}
			}

			// Nicely fade out and trim the item list if too big
			else if (TextItemsToAdd < 0)
			{
				for (int32 Index = ObjectList.Num(); Index < TextItemList.Num(); Index++)
				{
					TSharedPtr<SNovaHoverStackEntry>& Item = TextItemList[Index];
					if (!Item->GetText().IsEmpty())
					{
						Item->SetText(FText());
					}
					else
					{
						Container->RemoveSlot(Item.ToSharedRef());
						TextItemList.RemoveAt(Index);
					}
				}
			}

			// Build text for all objects
			int32 Index = 0;
			for (const FNovaOrbitalObject& Object : ObjectList)
			{
				TSharedPtr<SNovaHoverStackEntry>& Item = TextItemList[Index];

				bool InstantUpdate =
					Index < PreviousObjects.Num() && PreviousObjects[Index].Maneuver.IsValid() && Object.Maneuver.IsValid();

				Item->SetText(GetText(Object), InstantUpdate);
				Item->SetVisualEffects(Object.GetBrush(), GetColor(Object), Index == SelectedIndex);
				Index++;
			}
		}

		PreviousObjects = ObjectList;
	}

protected:

	TSharedPtr<SVerticalBox>                 Container;
	TArray<TSharedPtr<SNovaHoverStackEntry>> TextItemList;
	TArray<FNovaOrbitalObject>               PreviousObjects;
};

/*----------------------------------------------------
    Side panel container widget (fading)
----------------------------------------------------*/

class SNovaSidePanelContainer : public SNeutronFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaSidePanelContainer)
	{}

	SLATE_ARGUMENT(FSimpleDelegate, OnUpdate)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		// clang-format off
		SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments().Content()
		[
			InArgs._Content.Widget
		]);
		// clang-format on

		UpdateCallback = InArgs._OnUpdate;
		SetColorAndOpacity(TAttribute<FLinearColor>(this, &SNeutronFadingWidget::GetLinearColor));
	}

	void SetObject(FNovaOrbitalObject NewObject)
	{
		DesiredObject = NewObject;
	}

protected:

	virtual bool IsDirty() const override
	{
		return CurrentObject != DesiredObject;
	}

	virtual void OnUpdate() override
	{
		CurrentObject = DesiredObject;

		UpdateCallback.ExecuteIfBound();
	}

protected:

	FSimpleDelegate    UpdateCallback;
	FNovaOrbitalObject DesiredObject;
	FNovaOrbitalObject CurrentObject;
};

/*----------------------------------------------------
    Side panel widget (structure and animation)
----------------------------------------------------*/

class SNovaSidePanel : public SNeutronFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaSidePanel)
	{}

	SLATE_ARGUMENT(const SNeutronTabPanel*, Panel)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs)
	{
		const FNeutronMainTheme&   Theme       = FNeutronStyleSet::GetMainTheme();
		const FNeutronButtonTheme& ButtonTheme = FNeutronStyleSet::GetButtonTheme();

		// clang-format off
		SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments().Content()
		[
			SNew(SBorder)
			.BorderImage(&ButtonTheme.Border)
			.Padding(FMargin(0, 0, 1, 0))
			[
				SNew(SBackgroundBlur)
				.BlurRadius(InArgs._Panel, &SNeutronTabPanel::GetBlurRadius)
				.BlurStrength(InArgs._Panel, &SNeutronTabPanel::GetBlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBorder)
					.Padding(Theme.ContentPadding)
					.HAlign(HAlign_Center)
					[
						InArgs._Content.Widget
					]
				]
			]
		]);
		// clang-format on
	}

	void SetVisible(bool NewState)
	{
		DesiredState = NewState;
	}

	bool IsVisible() const
	{
		return CurrentState;
	}

	void Reset()
	{
		DesiredState = false;
		CurrentState = false;
		CurrentAlpha = 0.0f;
	}

	bool AreButtonsEnabled() const
	{
		return CurrentAlpha >= 1.0f;
	}

	FMargin GetMargin() const
	{
		float Alpha = CurrentState ? CurrentAlpha : 0.0f;
		return FMargin((Alpha - 1.0f) * 1000, 0, 0, 0);
	}

protected:

	virtual bool IsDirty() const override
	{
		return CurrentState != DesiredState;
	}

	virtual void OnUpdate() override
	{
		CurrentState = DesiredState;
	}

protected:

	bool DesiredState;
	bool CurrentState;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuNavigation::SNovaMainMenuNavigation()
	: PC(nullptr)
	, Spacecraft(nullptr)
	, GameState(nullptr)
	, OrbitalSimulation(nullptr)

	, HasHoveredObjects(false)
	, CurrentHoveredObjectIndex(0)
	, DestinationOrbit()
	, CurrentTrajectoryHasEnoughPropellant(false)
{}

void SNovaMainMenuNavigation::Construct(const FArguments& InArgs)
{
	// Data
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	GenericModalPanel = Menu->CreateModalPanel();

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		// Map slot
		+ SOverlay::Slot()
		[
			SAssignNew(OrbitalMap, SNovaOrbitalMap)
			.MenuManager(MenuManager)
			.CurrentAlpha_Lambda([this]()
			{
				return CurrentAlpha;
			})
		]

		// Main box
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.Padding(TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateLambda([&]()
		{
			return SidePanel->GetMargin();
		})))
		[
			SAssignNew(SidePanel, SNovaSidePanel)
			.Panel(this)
			[
				SAssignNew(SidePanelContainer, SNovaSidePanelContainer)
				.OnUpdate(FSimpleDelegate::CreateSP(this, &SNovaMainMenuNavigation::UpdateSidePanel))
				[
					SNew(SScrollBox)
					.Style(&Theme.ScrollBoxStyle)
					.ScrollBarVisibility(EVisibility::Collapsed)
					.AnimateWheelScrolling(true)

					// Header
					+ SScrollBox::Slot()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						[
							SNew(SBorder)
							.BorderImage(new FSlateNoResource)
							.Padding(Theme.VerticalContentPadding)
							.Visibility_Lambda([=]()
							{
								return DestinationTitle->GetText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
							})
							[
								SAssignNew(DestinationTitle, STextBlock)
								.TextStyle(&Theme.HeadingFont)
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNeutronNew(SNeutronButton)
							.Size("SmallButtonSize")
							.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Remove"))
							.HelpText(LOCTEXT("CloseSidePanelHelp", "Close the side panel"))
							.OnClicked(this, &SNovaMainMenuNavigation::OnHideSidePanel)
						]
					]

					// Description
					+ SScrollBox::Slot()
					[
						SNew(SBorder)
						.BorderImage(new FSlateNoResource)
						.Padding(Theme.VerticalContentPadding)
						.Visibility_Lambda([=]()
						{
							return DestinationDescription->GetText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
						})
						[
							SAssignNew(DestinationDescription, SRichTextBlock)
							.TextStyle(&Theme.MainFont)
							.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
							+ SRichTextBlock::ImageDecorator()
						]
					]
			
					// Trajectory computation widget
					+ SScrollBox::Slot()
					[
						SAssignNew(TrajectoryCalculator, SNovaTrajectoryCalculator)
						.MenuManager(MenuManager)
						.Panel(this)
						.FadeTime(0.0f)
						.CurrentAlpha_Lambda([=]()
						{
							return SidePanelContainer->GetCurrentAlpha();
						})
						.DeltaVActionName(FNeutronPlayerInput::MenuPrimary)
						.DurationActionName(FNeutronPlayerInput::MenuSecondary)
						.OnTrajectoryChanged(this, &SNovaMainMenuNavigation::OnTrajectoryChanged)
					]
			
					// Trajectory commit
					+ SScrollBox::Slot()
					.HAlign(HAlign_Center)
					[
						SNeutronAssignNew(CommitButton, SNeutronButton)
						.Size("DoubleButtonSize")
						.Icon(FNeutronStyleSet::GetBrush("Icon/SB_StartTrajectory"))
						.Text(LOCTEXT("StartFlightPlan", "Start flight plan"))
						.HelpText(this, &SNovaMainMenuNavigation::GetCommitTrajectoryHelpText)
						.OnClicked(this, &SNovaMainMenuNavigation::OnCommitTrajectory)
						.Enabled(this, &SNovaMainMenuNavigation::CanCommitTrajectory)
					]
			
					// Details
					+ SScrollBox::Slot()
					.HAlign(HAlign_Center)
					[
						SAssignNew(DestinationDetails, SVerticalBox)
					]
				]
			]
		]

		// Hover text slot
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		[
			SAssignNew(HoverStack, SNovaHoverStack)
		]

		// Price table
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		[
			SNeutronNew(SNeutronButton)
			.Action(FNeutronPlayerInput::MenuAltPrimary)
			.Text(LOCTEXT("ShowPriceTable", "Show price table"))
			.HelpText(LOCTEXT("ShowPriceTableHelp", "Vizualize the current price table for tradable resources"))
			.OnClicked(this, &SNovaMainMenuNavigation::ShowPriceTable)
			.Focusable(false)
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuNavigation::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Fetch currently hovered objects
	if (IsValid(GameState))
	{
		CurrentHoveredObjects = OrbitalMap->GetHoveredOrbitalObjects();
	}
	else
	{
		CurrentHoveredObjects.Empty();
	}
	CurrentHoveredObjectIndex = FMath::Min(CurrentHoveredObjectIndex, CurrentHoveredObjects.Num() - 1);
	CurrentHoveredObjectIndex = FMath::Max(CurrentHoveredObjectIndex, 0);

	// Show the tooltip
	if (CurrentHoveredObjects.Num())
	{
		if (!HasHoveredObjects)
		{
			MenuManager->ShowTooltip(this, LOCTEXT("OrbitalMapHint", "Inspect this object"));
			HasHoveredObjects = true;
		}
	}
	else
	{
		if (HasHoveredObjects)
		{
			MenuManager->HideTooltip(this);
			HasHoveredObjects = false;
		}
	}

	// Update the hover block
	HoverStack->Update(GameState, OrbitalMap, CurrentHoveredObjects, CurrentHoveredObjectIndex);
}

void SNovaMainMenuNavigation::Show()
{
	SNeutronTabPanel::Show();

	ResetTrajectory();
	SidePanel->Reset();
	OrbitalMap->Reset();
}

void SNovaMainMenuNavigation::Hide()
{
	SNeutronTabPanel::Hide();

	ResetTrajectory();
	SidePanel->Reset();
	OrbitalMap->Reset();
}

void SNovaMainMenuNavigation::UpdateGameObjects()
{
	PC                 = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	Spacecraft         = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	GameState          = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	AsteroidSimulation = IsValid(GameState) ? GameState->GetAsteroidSimulation() : nullptr;
	OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
}

void SNovaMainMenuNavigation::OnClicked(const FVector2D& Position)
{
	SNeutronTabPanel::OnClicked(Position);

	if (IsValid(GameState) && !SidePanel->IsHovered() && !OrbitalMap->GetPreviewTrajectory().IsValid())
	{
		bool HasValidObject      = false;
		bool HasAnyHoveredObject = false;

		// Check whether the current selection is relevant
		for (const FNovaOrbitalObject& Object : CurrentHoveredObjects)
		{
			HasAnyHoveredObject = true;

			if (Object.Area.IsValid() || Object.AsteroidIdentifier != FGuid() || Object.SpacecraftIdentifier != FGuid())
			{
				HasValidObject = true;
				break;
			}
		}

		// Case 1 : no existing selection, new valid selection (open)
		if (SelectedObject == FNovaOrbitalObject())
		{
			if (HasValidObject)
			{
				NCHECK(CurrentHoveredObjectIndex < CurrentHoveredObjects.Num());
				OnShowSidePanel(CurrentHoveredObjects[CurrentHoveredObjectIndex]);
			}
		}
		else
		{
			// Case 2 : existing selection, new valid selection (change)
			if (HasValidObject)
			{
				NCHECK(CurrentHoveredObjectIndex < CurrentHoveredObjects.Num());
				OnShowSidePanel(CurrentHoveredObjects[CurrentHoveredObjectIndex]);
			}

			// Case 3 : existing selection, no selection at all (do nothing)
			else if (!HasAnyHoveredObject)
			{
				SelectedObject = FNovaOrbitalObject();
			}
		}
	}
}

FReply SNovaMainMenuNavigation::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	FReply Reply = SNeutronTabPanel::OnKeyDown(MyGeometry, KeyEvent);

	if (MenuManager->IsUsingGamepad() && MenuManager->GetMenu()->IsActionKey(FNeutronPlayerInput::MenuConfirm, KeyEvent.GetKey()))
	{
		if (IsValid(GameState))
		{
			bool                       HasValidObject      = false;
			bool                       HasAnyHoveredObject = false;
			TArray<FNovaOrbitalObject> HoveredObjects      = CurrentHoveredObjects;

			// Check whether the current selection is relevant
			for (const FNovaOrbitalObject& Object : HoveredObjects)
			{
				HasAnyHoveredObject = true;

				if (Object.Area.IsValid() || Object.SpacecraftIdentifier != FGuid() || Object.AsteroidIdentifier != FGuid())
				{
					HasValidObject = true;
					break;
				}
			}

			//  No existing selection, new valid selection (open)
			if (SelectedObject == FNovaOrbitalObject() && HasValidObject)
			{
				NCHECK(CurrentHoveredObjectIndex < HoveredObjects.Num());
				OnShowSidePanel(HoveredObjects[CurrentHoveredObjectIndex]);

				return FReply::Handled();
			}
		}
	}
	else if (MenuManager->IsUsingGamepad() && MenuManager->GetMenu()->IsActionKey(FNeutronPlayerInput::MenuCancel, KeyEvent.GetKey()))
	{
		OnHideSidePanel();
	}

	return Reply;
}

void SNovaMainMenuNavigation::HorizontalAnalogInput(float Value)
{
	OrbitalMap->HorizontalAnalogInput(Value);
}

void SNovaMainMenuNavigation::VerticalAnalogInput(float Value)
{
	OrbitalMap->VerticalAnalogInput(Value);
}

void SNovaMainMenuNavigation::Next()
{
	CurrentHoveredObjectIndex = FMath::Min(CurrentHoveredObjectIndex + 1, CurrentHoveredObjects.Num());
}

void SNovaMainMenuNavigation::Previous()
{
	CurrentHoveredObjectIndex = FMath::Max(CurrentHoveredObjectIndex - 1, 0);
}

void SNovaMainMenuNavigation::ZoomIn()
{
	OrbitalMap->ZoomIn();
}

void SNovaMainMenuNavigation::ZoomOut()
{
	OrbitalMap->ZoomOut();
}

TSharedPtr<SNeutronButton> SNovaMainMenuNavigation::GetDefaultFocusButton() const
{
	return CommitButton;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void SNovaMainMenuNavigation::UpdateSidePanel()
{
	DestinationDetails->ClearChildren();

	const FNeutronMainTheme& Theme               = FNeutronStyleSet::GetMainTheme();
	const UNovaArea*         Area                = SelectedObject.Area.Get();
	const FNovaAsteroid*     Asteroid            = AsteroidSimulation->GetAsteroid(SelectedObject.AsteroidIdentifier);
	const FNovaSpacecraft*   TargetSpacecraft    = GameState->GetSpacecraft(SelectedObject.SpacecraftIdentifier);
	bool                     HasValidDestination = false;

	// Defaults
	TrajectoryCalculator->SetVisibility(EVisibility::Visible);
	CommitButton->SetVisibility(EVisibility::Visible);
	DestinationTitle->SetText(FText());
	DestinationDescription->SetText(FText());

	// Valid area found
	if (Area && ComputeTrajectoryTo(OrbitalSimulation->GetAreaOrbit(Area)))
	{
		NLOG("SNovaMainMenuNavigation::SelectDestination : %s", *Area->Name.ToString());

		HasValidDestination = true;
		DestinationTitle->SetText(Area->Name);
		DestinationDescription->SetText(Area->Description);

		// clang-format off
		DestinationDetails->AddSlot()
		.Padding(Theme.VerticalContentPadding)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SoldTitle", "Resources for sale here"))
			.TextStyle(&Theme.HeadingFont)
		];
		// clang-format on

		// Add sold resources
		for (const FNovaResourceTrade& Trade : Area->ResourceTradeMetadata)
		{
			if (Trade.ForSale)
			{
				// clang-format off
				DestinationDetails->AddSlot()
				.Padding(Theme.ContentPadding)
				.AutoHeight()
				[
					SNew(SNovaTradableAssetItem)
					.Area(Area)
					.Asset(Trade.Resource)
					.GameState(GameState)
					.Dark(true)
				];
				// clang-format on
			}
		}

		// clang-format off
		DestinationDetails->AddSlot()
		.Padding(Theme.VerticalContentPadding)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DealsTitle", "Best deals for your resources"))
			.TextStyle(&Theme.HeadingFont)
		];
		// clang-format on

		// Create a list of best deals for selling resources
		int32                                                   RemainingDealCount = 3;
		TArray<TPair<const UNovaResource*, ENovaPriceModifier>> BestDeals;
		for (const UNovaResource* Resource : Area->GetResourcesBought())
		{
			float CargoMass = Spacecraft->GetCargoMass(Resource);
			if (CargoMass > 0 && RemainingDealCount > 0)
			{
				BestDeals.Add(
					TPair<const UNovaResource*, ENovaPriceModifier>(Resource, GameState->GetCurrentPriceModifier(Resource, Area)));
				RemainingDealCount--;
			}
		}
		BestDeals.Sort(
			[&](const TPair<const UNovaResource*, ENovaPriceModifier>& A, const TPair<const UNovaResource*, ENovaPriceModifier>& B)
			{
				return B.Value < A.Value;
			});

		// Add the top N best deals
		if (BestDeals.Num() > 0)
		{
			int32 DealsToShow = 2;
			for (TPair<const UNovaResource*, ENovaPriceModifier>& Deal : BestDeals)
			{
				// clang-format off
				DestinationDetails->AddSlot()
				.Padding(Theme.ContentPadding)
				.AutoHeight()
				[
					SNew(SNovaTradableAssetItem)
					.Area(Area)
					.Asset(Deal.Key)
					.GameState(GameState)
					.Dark(true)
				];
				// clang-format on

				DealsToShow--;
				if (DealsToShow <= 0)
				{
					break;
				}
			}
		}
		else
		{
			// clang-format off
			DestinationDetails->AddSlot()
			.Padding(Theme.VerticalContentPadding)
			.AutoHeight()
			[
				SNew(STextBlock)
				.TextStyle(&Theme.MainFont)
				.Text(LOCTEXT("NoDealAvailable", "You have no resource to sell at this station"))
			];
			// clang-format on
		}
	}

	// Asteroid found
	if (Asteroid && ComputeTrajectoryTo(OrbitalSimulation->GetAsteroidOrbit(*Asteroid)))
	{
		NLOG("SNovaMainMenuNavigation::SelectDestination : %s", *Asteroid->Identifier.ToString(EGuidFormats::Short));

		HasValidDestination = true;
		DestinationTitle->SetText(FText::FromString(Asteroid->Identifier.ToString(EGuidFormats::Short).ToUpper()));
		DestinationDescription->SetText(LOCTEXT("Asteroid", "Asteroid"));
	}

	// Spacecraft found
	if (TargetSpacecraft)
	{
		NLOG("SNovaMainMenuNavigation::SelectDestination : %s", *SelectedObject.SpacecraftIdentifier.ToString(EGuidFormats::Short));

		HasValidDestination = true;
		DestinationTitle->SetText(TargetSpacecraft->GetName());
		TrajectoryCalculator->SetVisibility(EVisibility::Collapsed);
		CommitButton->SetVisibility(EVisibility::Collapsed);

		// Set the class text
		FText TargetSpacecraftClass;
		if (TargetSpacecraft->SpacecraftClass)
		{
			TargetSpacecraftClass =
				FText::FormatNamed(LOCTEXT("SpacecraftClassFormat", "{class}-class {classification}"), TEXT("classification"),
					TargetSpacecraft->GetClassification().ToLower(), TEXT("class"), TargetSpacecraft->SpacecraftClass->Name);

			// clang-format off
			DestinationDetails->AddSlot()
			.Padding(Theme.VerticalContentPadding)
			.AutoHeight()
			[
				SNew(SScaleBox)
				[
					SNew(SImage)
					.Image(&TargetSpacecraft->SpacecraftClass->AssetRender)
				]
			];
			// clang-format on
		}
		else
		{
			TargetSpacecraftClass = TargetSpacecraft->GetClassification();
		}
		DestinationDescription->SetText(TargetSpacecraftClass);
	}

	// No valid area found
	if (!HasValidDestination)
	{
		ResetTrajectory();
	}
}

bool SNovaMainMenuNavigation::ComputeTrajectoryTo(const FNovaOrbit& Orbit)
{
	if (HasValidSpacecraft() && !OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier()))
	{
		DestinationOrbit = Orbit;

		const FNovaOrbit* CurrentOrbit = OrbitalSimulation->GetPlayerOrbit();

		if (CurrentOrbit && Orbit != *CurrentOrbit)
		{
			TrajectoryCalculator->SimulateTrajectories(
				*OrbitalSimulation->GetPlayerOrbit(), Orbit, GameState->GetPlayerSpacecraftIdentifiers());

			TrajectoryCalculator->SetVisibility(EVisibility::Visible);
			CommitButton->SetVisibility(EVisibility::Visible);
		}
		else
		{
			TrajectoryCalculator->SetVisibility(EVisibility::Collapsed);
			CommitButton->SetVisibility(EVisibility::Collapsed);
		}

		return true;
	}
	else
	{
		return false;
	}
}

void SNovaMainMenuNavigation::ResetTrajectory()
{
	NLOG("SNovaMainMenuNavigation::ResetDestination");

	DestinationOrbit = FNovaOrbit();
	SelectedObject   = FNovaOrbitalObject();

	OrbitalMap->ClearTrajectory();
	TrajectoryCalculator->Reset();

	SidePanel->SetVisible(false);
	SidePanelContainer->SetObject({});
}

bool SNovaMainMenuNavigation::HasValidSpacecraft() const
{
	if (IsValid(GameState) && IsValid(OrbitalSimulation))
	{
		for (FGuid Identifier : GameState->GetPlayerSpacecraftIdentifiers())
		{
			const FNovaSpacecraft* PlayerSpacecraft = GameState->GetSpacecraft(Identifier);
			if (PlayerSpacecraft == nullptr || !PlayerSpacecraft->IsValid())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

bool SNovaMainMenuNavigation::CanCommitTrajectoryInternal(FText* Details) const
{
	if (IsValid(OrbitalSimulation) && IsValid(GameState))
	{
		const FNovaOrbit* CurrentOrbit = OrbitalSimulation->GetPlayerOrbit();
		if (CurrentOrbit && DestinationOrbit == *CurrentOrbit)
		{
			if (Details)
			{
				*Details = LOCTEXT("CanCommitFlightPlanIdentical", "The selected flight plan goes to your current location");
			}
			return false;
		}
		else if (!CurrentTrajectoryHasEnoughPropellant)
		{
			if (Details)
			{
				*Details = FText::FormatNamed(
					LOCTEXT("OneSpacecraftLacksPropellant",
						"{spacecraft}|plural(one=This,other=A) spacecraft doesn't have enough propellant for this flight plan"),
					TEXT("spacecraft"), GameState->GetPlayerSpacecraftIdentifiers().Num());
			}

			return false;
		}
		else if (!OrbitalSimulation->CanCommitTrajectory(OrbitalMap->GetPreviewTrajectory(), Details))
		{
			return false;
		}
	}

	return SidePanel->AreButtonsEnabled();
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuNavigation::CanCommitTrajectory() const
{
	return CanCommitTrajectoryInternal();
}

FText SNovaMainMenuNavigation::GetCommitTrajectoryHelpText() const
{
	FText Help;
	if (CanCommitTrajectoryInternal(&Help))
	{
		return LOCTEXT("CommitFlightPlanHelp", "Commit to the currently selected flight plan");
	}
	else
	{
		return Help;
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuNavigation::ShowPriceTable()
{
	const UNeutronAssetManager* AssetManager = UNeutronAssetManager::Get();
	NCHECK(AssetManager);

	// Get formating tools
	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 1;
	const FNeutronMainTheme& Theme  = FNeutronStyleSet::GetMainTheme();

	// This is an unfortunate limitation of tables, don't forget to change here the number of areas supported
	TArray<const UNovaArea*> Areas     = AssetManager->GetAssets<UNovaArea>();
	static constexpr uint8   AreaCount = 3;
	NCHECK(Areas.Num() == AreaCount);

	// Create the table structure
	// clang-format off
	TSharedPtr<SNeutronTable<AreaCount>> PriceTable;
	TSharedPtr<SWidget>                  TableContainer =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(PriceTable, SNeutronTable<AreaCount>)
			.Title(LOCTEXT("PriceTableTitle", "Current resource prices"))
			.Width(2 * Theme.GenericMenuWidth)
		]
		+ SHorizontalBox::Slot();
	// clang-format on

	// Add the headers
	TArray<FText> AreaNames;
	for (const UNovaArea* Area : Areas)
	{
		AreaNames.Add(Area->Name);
	}
	PriceTable->AddHeaders(LOCTEXT("Resource", "Resource"), AreaNames);

	// Iterate all resources for each area
	for (const UNovaResource* Resource : AssetManager->GetAssets<UNovaResource>())
	{
		TArray<TNeutronTableValue<FText>> PriceList;
		for (const UNovaArea* Area : Areas)
		{
			const FNovaCredits       Price    = GameState->GetCurrentPrice(Resource, Area, false);
			const ENovaPriceModifier Modifier = GameState->GetCurrentPriceModifier(Resource, Area);

			// Process the price modifier
			FLinearColor ModifierColor;
			FText        IconText;
			FText        ModifierText;
			switch (Modifier)
			{
				case ENovaPriceModifier::Cheap:
					IconText      = INVTEXT("Cheap");
					ModifierText  = LOCTEXT("Cheap", "Cheap");
					ModifierColor = Theme.NegativeColor;
					break;
				case ENovaPriceModifier::BelowAverage:
					IconText      = INVTEXT("BelowAverage");
					ModifierText  = LOCTEXT("BelowAverage", "Below average");
					ModifierColor = Theme.NegativeColor;
					break;
				case ENovaPriceModifier::Average:
					IconText      = INVTEXT("Average");
					ModifierText  = LOCTEXT("Average", "Average");
					ModifierColor = Theme.NeutralColor;
					break;
				case ENovaPriceModifier::AboveAverage:
					IconText      = INVTEXT("AboveAverage");
					ModifierText  = LOCTEXT("AboveAverage", "Above average");
					ModifierColor = Theme.PositiveColor;
					break;
				case ENovaPriceModifier::Expensive:
					IconText      = INVTEXT("Expensive");
					ModifierText  = LOCTEXT("Expensive", "Expensive");
					ModifierColor = Theme.PositiveColor;
					break;
			}

			// Mark if the resource is for sale there
			FText SoldText;
			if (Area->GetResourcesSold().Contains(Resource))
			{
				SoldText = FText::FromString(TEXT("\n") + LOCTEXT("SoldThere", "Selling").ToString());
			}
			else
			{
				SoldText = FText::FromString(TEXT("\n") + LOCTEXT("BoughtThere", "Buying").ToString());
			}

			PriceList.Add(
				TNeutronTableValue(FText::FormatNamed(INVTEXT("{price} (<img src=\"/Text/{icon}\"/>{modifier}){sold}"), TEXT("price"),
									   GetPriceText(Price), TEXT("icon"), IconText, TEXT("modifier"), ModifierText, TEXT("sold"), SoldText),
					INVTEXT("Ѥ"), ModifierColor));
		}

		PriceTable->AddEntries(Resource->Name, PriceList);
	}

	GenericModalPanel->Show(
		LOCTEXT("PriceTable", "Price table"), FText(), FSimpleDelegate(), FSimpleDelegate(), FSimpleDelegate(), TableContainer);
}

void SNovaMainMenuNavigation::OnShowSidePanel(const FNovaOrbitalObject& HoveredObject)
{
	NLOG("SNovaMainMenuNavigation::OnShowSidePanel");

	SelectedObject = HoveredObject;
	SidePanel->SetVisible(true);
	SidePanelContainer->SetObject(SelectedObject);
}

void SNovaMainMenuNavigation::OnHideSidePanel()
{
	if (SidePanel->IsVisible())
	{
		NLOG("SNovaMainMenuNavigation::OnHideSidePanel");

		SelectedObject = FNovaOrbitalObject();
		SidePanel->SetVisible(false);
		SidePanelContainer->SetObject(SelectedObject);
	}
}

void SNovaMainMenuNavigation::OnTrajectoryChanged(const FNovaTrajectory& Trajectory, bool HasEnoughPropellant)
{
	CurrentTrajectoryHasEnoughPropellant = HasEnoughPropellant;

	if (Trajectory.IsValidExtended())
	{
		TArray<FNovaOrbitalObject> Objects;
		Objects.Add(SelectedObject);
		for (FGuid Identifier : GameState->GetPlayerSpacecraftIdentifiers())
		{
			Objects.Add(FNovaOrbitalObject(Identifier, FVector2D::ZeroVector, false));
		}

		OrbitalMap->ShowTrajectory(Trajectory, Objects, false);
	}
	else
	{
		OrbitalMap->ClearTrajectory();
	}
}

void SNovaMainMenuNavigation::OnCommitTrajectory()
{
	const FNovaTrajectory& Trajectory = OrbitalMap->GetPreviewTrajectory();
	if (IsValid(OrbitalSimulation) && OrbitalSimulation->CanCommitTrajectory(Trajectory))
	{
		OrbitalSimulation->CommitTrajectory(GameState->GetPlayerSpacecraftIdentifiers(), Trajectory);

		ResetTrajectory();
	}
}

#undef LOCTEXT_NAMESPACE
