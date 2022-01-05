// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuNavigation.h"

#include "NovaMainMenu.h"

#include "Game/NovaArea.h"
#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "System/NovaAssetManager.h"
#include "System/NovaGameInstance.h"
#include "System/NovaMenuManager.h"

#include "Player/NovaPlayerController.h"
#include "UI/Component/NovaTrajectoryCalculator.h"
#include "UI/Component/NovaTradableAssetItem.h"
#include "UI/Widget/NovaFadingWidget.h"
#include "UI/Widget/NovaSlider.h"
#include "Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuNavigation"

#define STACK_PANEL_WIDTH 500

/*----------------------------------------------------
    Hover details stack
----------------------------------------------------*/

/** Fading text widget with a background, inheriting SNovaFadingWidget for simplicity */
class SNovaHoverStackEntry : public SNovaText
{
public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		SNovaText::Construct(SNovaText::FArguments()
								 .TextStyle(&Theme.MainFont)
								 .WrapTextAt(STACK_PANEL_WIDTH - Theme.ContentPadding.Left - Theme.ContentPadding.Right - 24));

		// clang-format off
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBackground)
			.Padding(Theme.ContentPadding)
			.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
			[
				SNew(SHorizontalBox)

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

	void SetBrushAndColor(const FSlateBrush* NewBrush, const FLinearColor NewColor)
	{
		Brush = NewBrush;
		Color = NewColor;
	}

protected:
	const FSlateBrush* Brush;
	FLinearColor       Color;
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
		// clang-format off
		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(STACK_PANEL_WIDTH)
			[
				SAssignNew(Container, SVerticalBox)
			]
		];
		// clang-format on
	}

public:
	void SetObjectList(const ANovaGameState* GameState, TSharedPtr<SNovaOrbitalMap> OrbitalMap, TArray<FNovaOrbitalObject> ObjectList)
	{
		if (IsValid(GameState))
		{
			const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

			// Text builder
			auto GetText = [GameState](const FNovaOrbitalObject& Object)
			{
				if (Object.Area.IsValid())
				{
					return Object.Area->Name;
				}
				else if (Object.AsteroidIdentifier != FGuid())
				{
					return FText::FromString(Object.AsteroidIdentifier.ToString(EGuidFormats::Short));
				}
				else if (Object.SpacecraftIdentifier != FGuid())
				{
					const FNovaSpacecraft* Spacecraft = GameState->GetSpacecraft(Object.SpacecraftIdentifier);
					NCHECK(Spacecraft);
					return Spacecraft->GetName();
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
				Item->SetBrushAndColor(Object.GetBrush(), GetColor(Object));
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

class SNovaSidePanelContainer : public SNovaFadingWidget<false>
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
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments().Content()
		[
			InArgs._Content.Widget
		]);
		// clang-format on

		UpdateCallback = InArgs._OnUpdate;
		SetColorAndOpacity(TAttribute<FLinearColor>(this, &SNovaFadingWidget::GetLinearColor));
	}

	void SetObjectList(TArray<FNovaOrbitalObject> NewObjectList)
	{
		DesiredObjectList = NewObjectList;
	}

protected:
	virtual bool IsDirty() const override
	{
		if (CurrentObjectList.Num() != DesiredObjectList.Num())
		{
			return true;
		}
		else
		{
			for (int32 i = 0; i < CurrentObjectList.Num(); i++)
			{
				if (CurrentObjectList[i] != DesiredObjectList[i])
				{
					return true;
				}
			}
		}

		return false;
	}

	virtual void OnUpdate() override
	{
		CurrentObjectList = DesiredObjectList;

		UpdateCallback.ExecuteIfBound();
	}

protected:
	FSimpleDelegate            UpdateCallback;
	TArray<FNovaOrbitalObject> DesiredObjectList;
	TArray<FNovaOrbitalObject> CurrentObjectList;
};

/*----------------------------------------------------
    Side panel widget (structure and animation)
----------------------------------------------------*/

class SNovaSidePanel : public SNovaFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaSidePanel)
	{}

	SLATE_ARGUMENT(const SNovaTabPanel*, Panel)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme&   Theme       = FNovaStyleSet::GetMainTheme();
		const FNovaButtonTheme& ButtonTheme = FNovaStyleSet::GetButtonTheme();

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments().Content()
		[
			SNew(SBorder)
			.BorderImage(&ButtonTheme.Border)
			.Padding(FMargin(0, 0, 1, 0))
			[
				SNew(SBackgroundBlur)
				.BlurRadius(InArgs._Panel, &SNovaTabPanel::GetBlurRadius)
				.BlurStrength(InArgs._Panel, &SNovaTabPanel::GetBlurStrength)
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
	, SelectedDestination(nullptr)
{}

void SNovaMainMenuNavigation::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)

		// Map slot
		+ SOverlay::Slot()
		[
			SAssignNew(OrbitalMap, SNovaOrbitalMap)
			.MenuManager(MenuManager)
			.CurrentAlpha(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([=]()
			{
				return CurrentAlpha;
			})))
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
							.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([=]()
							{
								return AreaTitle->GetText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
							})))
							[
								SAssignNew(AreaTitle, STextBlock)
								.TextStyle(&Theme.HeadingFont)
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNovaNew(SNovaButton)
							.Size("SmallButtonSize")
							.Icon(FNovaStyleSet::GetBrush("Icon/SB_Remove"))
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
						.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([=]()
						{
							return AreaDescription->GetText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
						})))
						[
							SAssignNew(AreaDescription, STextBlock)
							.TextStyle(&Theme.MainFont)
						]
					]
			
					// Trajectory computation widget
					+ SScrollBox::Slot()
					[
						SAssignNew(TrajectoryCalculator, SNovaTrajectoryCalculator)
						.MenuManager(MenuManager)
						.Panel(this)
						.FadeTime(0.0f)
						.CurrentAlpha(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([=]()
						{
							return SidePanelContainer->GetCurrentAlpha();
						})))
						.DeltaVActionName(FNovaPlayerInput::MenuPrimary)
						.DurationActionName(FNovaPlayerInput::MenuSecondary)
						.OnTrajectoryChanged(this, &SNovaMainMenuNavigation::OnTrajectoryChanged)
					]
			
					// Trajectory commit
					+ SScrollBox::Slot()
					.HAlign(HAlign_Center)
					[
						SNovaAssignNew(CommitButton, SNovaButton)
						.Size("DoubleButtonSize")
						.Text(LOCTEXT("StartFlightPlan", "Start flight plan"))
						.HelpText(this, &SNovaMainMenuNavigation::GetCommitTrajectoryHelpText)
						.OnClicked(this, &SNovaMainMenuNavigation::OnCommitTrajectory)
						.Enabled(this, &SNovaMainMenuNavigation::CanCommitTrajectory)
					]
			
					+ SScrollBox::Slot()
					.HAlign(HAlign_Center)
					[
						SAssignNew(StationTrades, SVerticalBox)
					]
				]
			]
		]

		// Hover text slot
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		[
			SAssignNew(HoverText, SNovaHoverStack)
		]
	];
	// clang-format on
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuNavigation::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Fetch currently hovered objects
	TArray<FNovaOrbitalObject> CurrentHoveredObjects;
	if (IsValid(GameState))
	{
		CurrentHoveredObjects = OrbitalMap->GetHoveredOrbitalObjects();
	}

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
	HoverText->SetObjectList(GameState, OrbitalMap, CurrentHoveredObjects);
}

void SNovaMainMenuNavigation::Show()
{
	SNovaTabPanel::Show();

	ResetDestination();
	SidePanel->Reset();
	OrbitalMap->Reset();
}

void SNovaMainMenuNavigation::Hide()
{
	SNovaTabPanel::Hide();

	ResetDestination();
	SidePanel->Reset();
	OrbitalMap->Reset();
}

void SNovaMainMenuNavigation::UpdateGameObjects()
{
	PC                = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	Spacecraft        = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	GameState         = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	OrbitalSimulation = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
}

void SNovaMainMenuNavigation::OnClicked(const FVector2D& Position)
{
	SNovaTabPanel::OnClicked(Position);

	if (IsValid(GameState) && !SidePanel->IsHovered())
	{
		bool                       HasValidObject      = false;
		bool                       HasAnyHoveredObject = false;
		TArray<FNovaOrbitalObject> HoveredObjects      = OrbitalMap->GetHoveredOrbitalObjects();

		// Check whether the current selection is relevant
		for (const FNovaOrbitalObject& Object : HoveredObjects)
		{
			HasAnyHoveredObject = true;

			if (Object.Area.IsValid() || Object.SpacecraftIdentifier != FGuid())
			{
				HasValidObject = true;
				break;
			}
		}

		// Case 1 : no existing selection, new valid selection (open)
		if (SelectedObjectList.Num() == 0)
		{
			if (HasValidObject)
			{
				OnShowSidePanel(HoveredObjects);
			}
		}
		else
		{
			// Case 2 : existing selection, new valid selection (change)
			if (HasValidObject)
			{
				OnShowSidePanel(HoveredObjects);
			}

			// Case 3 : existing selection, no selection at all (do nothing)
			else if (!HasAnyHoveredObject)
			{
				SelectedObjectList = {};
			}
		}
	}
}

FReply SNovaMainMenuNavigation::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Result = SNovaTabPanel::OnKeyDown(MyGeometry, InKeyEvent);

	if (MenuManager->IsUsingGamepad() && MenuManager->GetMenu()->IsActionKey(FNovaPlayerInput::MenuConfirm, InKeyEvent.GetKey()))
	{
		if (IsValid(GameState))
		{
			bool                       HasValidObject      = false;
			bool                       HasAnyHoveredObject = false;
			TArray<FNovaOrbitalObject> HoveredObjects      = OrbitalMap->GetHoveredOrbitalObjects();

			// Check whether the current selection is relevant
			for (const FNovaOrbitalObject& Object : HoveredObjects)
			{
				HasAnyHoveredObject = true;

				if (Object.Area.IsValid() || Object.SpacecraftIdentifier != FGuid())
				{
					HasValidObject = true;
					break;
				}
			}

			//  No existing selection, new valid selection (open)
			if (SelectedObjectList.Num() == 0 && HasValidObject)
			{
				OnShowSidePanel(HoveredObjects);

				return FReply::Handled();
			}
		}
	}
	else if (MenuManager->IsUsingGamepad() && MenuManager->GetMenu()->IsActionKey(FNovaPlayerInput::MenuCancel, InKeyEvent.GetKey()))
	{
		OnHideSidePanel();

		return FReply::Handled();
	}

	return Result;
}

void SNovaMainMenuNavigation::HorizontalAnalogInput(float Value)
{
	OrbitalMap->HorizontalAnalogInput(Value);
}

void SNovaMainMenuNavigation::VerticalAnalogInput(float Value)
{
	OrbitalMap->VerticalAnalogInput(Value);
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void SNovaMainMenuNavigation::UpdateSidePanel()
{
	bool HasValidDestination = false;

	StationTrades->ClearChildren();
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	for (const FNovaOrbitalObject& Object : SelectedObjectList)
	{
		// Valid area found
		if (Object.Area.IsValid() && SelectDestination(Object.Area.Get()))
		{
			HasValidDestination = true;
			AreaTitle->SetText(Object.Area->Name);
			AreaDescription->SetText(Object.Area->Description);

			// clang-format off
			StationTrades->AddSlot()
			.Padding(Theme.VerticalContentPadding)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SoldTitle", "For sale here"))
				.TextStyle(&Theme.HeadingFont)
			];
			// clang-format on

			// Add sold resources
			for (const FNovaResourceTrade& Trade : Object.Area->ResourceTradeMetadata)
			{
				if (Trade.ForSale)
				{
					// clang-format off
					StationTrades->AddSlot()
					.Padding(Theme.ContentPadding)
					.AutoHeight()
					[
						SNew(SNovaTradableAssetItem)
						.Asset(Trade.Resource)
						.GameState(GameState)
						.Dark(true)
					];
					// clang-format on
				}
			}

			// clang-format off
			// TODO add deals here
			/*StationTrades->AddSlot()
			.Padding(Theme.VerticalContentPadding)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DealsTitle", "Best deals"))
				.TextStyle(&Theme.HeadingFont)
			];*/
			// clang-format on

			// Create a list of best deals for selling resources
			TArray<TPair<const UNovaResource*, ENovaPriceModifier>> BestDeals;
			for (int32 Index = 0; Index < Spacecraft->Compartments.Num(); Index++)
			{
				auto AddDeal = [this, &BestDeals, &Object, Index](ENovaResourceType Type)
				{
					const FNovaSpacecraftCargo& Cargo = Spacecraft->Compartments[Index].GetCargo(Type);
					if (Cargo.Amount > 0 && !GameState->IsResourceSold(Cargo.Resource, Object.Area.Get()))
					{
						BestDeals.Add(TPair<const UNovaResource*, ENovaPriceModifier>(
							Cargo.Resource, GameState->GetCurrentPriceModifier(Cargo.Resource)));
					}
				};

				AddDeal(ENovaResourceType::General);
				AddDeal(ENovaResourceType::Bulk);
				AddDeal(ENovaResourceType::Liquid);
			}

			BestDeals.Sort(
				[&](const TPair<const UNovaResource*, ENovaPriceModifier>& A, const TPair<const UNovaResource*, ENovaPriceModifier>& B)
				{
					return B.Value < A.Value;
				});

			// Add the N top deals
			int32 DealsToShow = 3;
			for (TPair<const UNovaResource*, ENovaPriceModifier>& Deal : BestDeals)
			{
				// clang-format off
					StationTrades->AddSlot()
					.Padding(Theme.ContentPadding)
					.AutoHeight()
					[
						SNew(SNovaTradableAssetItem)
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
	}

	// No valid area found
	if (!HasValidDestination)
	{
		ResetDestination();
		AreaTitle->SetText(FText());
	}
}

bool SNovaMainMenuNavigation::SelectDestination(const UNovaArea* Destination)
{
	NLOG("SNovaMainMenuNavigation::SelectDestination : %s", *Destination->Name.ToString());

	if (CanSelectDestination(Destination))
	{
		SelectedDestination = Destination;

		if (Destination != GameState->GetCurrentArea())
		{
			TrajectoryCalculator->SimulateTrajectories(MakeShared<FNovaOrbit>(*OrbitalSimulation->GetPlayerOrbit()),
				OrbitalSimulation->GetAreaOrbit(Destination), GameState->GetPlayerSpacecraftIdentifiers());

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

void SNovaMainMenuNavigation::ResetDestination()
{
	NLOG("SNovaMainMenuNavigation::ResetDestination");

	SelectedDestination = nullptr;
	SelectedObjectList  = {};

	OrbitalMap->ClearTrajectory();
	TrajectoryCalculator->Reset();

	SidePanel->SetVisible(false);
	SidePanelContainer->SetObjectList({});
}

bool SNovaMainMenuNavigation::CanSelectDestination(const UNovaArea* Destination) const
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
		if (SelectedDestination == GameState->GetCurrentArea())
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
						"{spacecraft}|plural(one=The,other=A) spacecraft doesn't have enough propellant for this trajectory"),
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

void SNovaMainMenuNavigation::OnShowSidePanel(const TArray<FNovaOrbitalObject>& HoveredObjects)
{
	NLOG("SNovaMainMenuNavigation::OnShowSidePanel");

	SelectedObjectList = HoveredObjects;
	SidePanel->SetVisible(true);
	SidePanelContainer->SetObjectList(SelectedObjectList);
}

void SNovaMainMenuNavigation::OnHideSidePanel()
{
	if (SidePanel->IsVisible())
	{
		NLOG("SNovaMainMenuNavigation::OnHideSidePanel");

		SelectedObjectList = {};
		SidePanel->SetVisible(false);
		SidePanelContainer->SetObjectList({});
	}
}

void SNovaMainMenuNavigation::OnTrajectoryChanged(TSharedPtr<FNovaTrajectory> Trajectory, bool HasEnoughPropellant)
{
	CurrentTrajectoryHasEnoughPropellant = HasEnoughPropellant;

	if (Trajectory.IsValid())
	{
		OrbitalMap->ShowTrajectory(Trajectory, false);
	}
	else
	{
		OrbitalMap->ClearTrajectory();
	}
}

void SNovaMainMenuNavigation::OnCommitTrajectory()
{
	const TSharedPtr<FNovaTrajectory>& Trajectory = OrbitalMap->GetPreviewTrajectory();
	if (IsValid(OrbitalSimulation) && OrbitalSimulation->CanCommitTrajectory(Trajectory))
	{
		OrbitalSimulation->CommitTrajectory(GameState->GetPlayerSpacecraftIdentifiers(), Trajectory);

		ResetDestination();
	}
}

#undef LOCTEXT_NAMESPACE
