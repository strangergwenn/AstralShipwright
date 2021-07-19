// Nova project - Gwennaël Arbona

#include "NovaMainMenuNavigation.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/System/NovaSpacecraftPropellantSystem.h"

#include "Nova/System/NovaAssetManager.h"
#include "Nova/System/NovaGameInstance.h"
#include "Nova/System/NovaMenuManager.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/UI/Component/NovaTrajectoryCalculator.h"
#include "Nova/UI/Component/NovaTradableAssetItem.h"
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaSlider.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuNavigation"

/*----------------------------------------------------
    Hover details stack
----------------------------------------------------*/

/** Fading text widget with a background */
class SNovaHoverStackEntry : public SNovaText
{
public:
	void Construct(const FArguments& InArgs)
	{
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
		SNovaText::Construct(SNovaText::FArguments().TextStyle(&Theme.MainFont).WrapTextAt(500));

		// clang-format off
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuBackground)
			.Padding(Theme.ContentPadding)
			.BorderBackgroundColor(this, &SNovaFadingWidget::GetSlateColor)
			[
				TextBlock.ToSharedRef()
			]
		];
		// clang-format on
	}
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
			.WidthOverride(500)
			[
				SAssignNew(Container, SVerticalBox)
			]
		];
		// clang-format on
	}

public:
	void SetObjectList(const ANovaGameState* GameState, TArray<FNovaOrbitalObject> ObjectList)
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
					FText     ManeuverTextFormat =
                        TimeLeftBeforeManeuver > 0
								? LOCTEXT("ManeuverFormatValid", "{duration} burn for a {deltav} m/s maneuver at {phase}° in {time}")
								: LOCTEXT("ManeuverFormatExpired", "{duration} burn for a {deltav} m/s maneuver at {phase}°");

					return FText::FormatNamed(ManeuverTextFormat, TEXT("phase"),
						FText::AsNumber(FMath::Fmod(Object.Maneuver->Phase, 360.0f), &NumberOptions), TEXT("time"),
						GetDurationText(TimeLeftBeforeManeuver), TEXT("duration"), GetDurationText(Object.Maneuver->Duration),
						TEXT("deltav"), FText::AsNumber(Object.Maneuver->DeltaV, &NumberOptions));
				}
				else
				{
					return FText();
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

				FText Text = FText::FromString(FString(TEXT("• ")) + GetText(Object).ToString());
				bool  InstantUpdate =
					Index < PreviousObjects.Num() && PreviousObjects[Index].Maneuver.IsValid() && Object.Maneuver.IsValid();

				Item->SetText(Text, InstantUpdate);
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
		ColorAndOpacity.BindRaw(this, &SNovaFadingWidget::GetLinearColor);
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
		const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

		// clang-format off
		SNovaFadingWidget::Construct(SNovaFadingWidget::FArguments().Content()
		[
			SNew(SBox)
			.WidthOverride(500)
			[
				SNew(SBackgroundBlur)
				.BlurRadius(InArgs._Panel, &SNovaTabPanel::GetBlurRadius)
				.BlurStrength(InArgs._Panel, &SNovaTabPanel::GetBlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuBackground)
					.Padding(Theme.ContentPadding)
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
	, SpacecraftPawn(nullptr)
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
			
					// Delta-v trade-off slider
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
						.DurationActionName(FNovaPlayerInput::MenuPrimary)
						.DeltaVActionName(FNovaPlayerInput::MenuSecondary)
						.OnTrajectoryChanged(this, &SNovaMainMenuNavigation::OnTrajectoryChanged)
					]

					// Fuel use
					+ SScrollBox::Slot()
					.HAlign(HAlign_Center)
					[
						SAssignNew(FuelText, STextBlock)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([&]()
						{
							if (IsValid(GameState) && IsValid(OrbitalSimulation) && IsValid(SpacecraftPawn) && CurrentSimulatedTrajectory.IsValid())
							{
								UNovaSpacecraftPropellantSystem* PropellantSystem = SpacecraftPawn->FindComponentByClass<UNovaSpacecraftPropellantSystem>();
								NCHECK(PropellantSystem);
								float FuelToBeUsed = OrbitalSimulation->GetPlayerRemainingFuelRequired(*CurrentSimulatedTrajectory, SpacecraftPawn->GetSpacecraftIdentifier());
								float FuelRemaining = PropellantSystem->GetCurrentPropellantMass();

								FNumberFormattingOptions Options;
								Options.MaximumFractionalDigits = 1;

								return FText::FormatNamed(LOCTEXT("TrajectoryFuelFormat", "{used} T of propellant will be used ({remaining} T remaining)"),
									TEXT("used"), FText::AsNumber(FuelToBeUsed, &Options),
									TEXT("remaining"), FText::AsNumber(FuelRemaining, &Options));
							}

							return FText();
						})))
						.TextStyle(&Theme.MainFont)
						.WrapTextAt(500)
					]
			
					+ SScrollBox::Slot()
					.HAlign(HAlign_Center)
					[
						SNovaAssignNew(CommitButton, SNovaButton)
						.Size("WideButtonSize")
						.Text(LOCTEXT("CommitTrajectory", "Commit trajectory"))
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
	HoverText->SetObjectList(GameState, CurrentHoveredObjects);
}

void SNovaMainMenuNavigation::Show()
{
	SNovaTabPanel::Show();

	ResetDestination();
	SidePanel->Reset();
}

void SNovaMainMenuNavigation::Hide()
{
	SNovaTabPanel::Hide();

	ResetDestination();
	SidePanel->Reset();
}

void SNovaMainMenuNavigation::UpdateGameObjects()
{
	PC                = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	Spacecraft        = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	SpacecraftPawn    = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	GameState         = IsValid(PC) ? MenuManager->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	OrbitalSimulation = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
}

void SNovaMainMenuNavigation::OnClicked(const FVector2D& Position)
{
	SNovaTabPanel::OnClicked(Position);

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

		// Case 1 : no existing selection, new valid selection (open)
		if (SelectedObjectList.Num() == 0)
		{
			if (HasValidObject)
			{
				SelectedObjectList = HoveredObjects;
				SidePanel->SetVisible(true);
				SidePanelContainer->SetObjectList(SelectedObjectList);
			}
		}
		else
		{
			// Case 2 : existing selection, new valid selection (change)
			if (HasValidObject)
			{
				SelectedObjectList = HoveredObjects;
				SidePanelContainer->SetObjectList(SelectedObjectList);
			}

			// Case 3 : existing selection, no selection at all (close)
			else if (!HasAnyHoveredObject)
			{
				SelectedObjectList = {};
				SidePanel->SetVisible(false);
				SidePanelContainer->SetObjectList({});
			}
		}
	}
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

			// clang-format off
			StationTrades->AddSlot()
			.Padding(Theme.VerticalContentPadding)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SoldTitle", "For sale"))
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
			StationTrades->AddSlot()
			.Padding(Theme.VerticalContentPadding)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DealsTitle", "Best deals"))
				.TextStyle(&Theme.HeadingFont)
			];
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
			FuelText->SetVisibility(EVisibility::Visible);
			CommitButton->SetVisibility(EVisibility::Visible);
		}
		else
		{
			TrajectoryCalculator->SetVisibility(EVisibility::Collapsed);
			FuelText->SetVisibility(EVisibility::Collapsed);
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
	CurrentSimulatedTrajectory.Reset();
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
				*Details = LOCTEXT("CanCommitTrajectoryIdentical", "The selected trajectory goes to your current location");
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
		return LOCTEXT("CommitTrajectoryHelp", "Commit to the currently selected trajectory");
	}
	else
	{
		return Help;
	}
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuNavigation::OnTrajectoryChanged(TSharedPtr<FNovaTrajectory> Trajectory)
{
	CurrentSimulatedTrajectory = Trajectory;
	OrbitalMap->ShowTrajectory(Trajectory, false);
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
