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
#include "Nova/UI/Widget/NovaFadingWidget.h"
#include "Nova/UI/Widget/NovaSlider.h"
#include "Nova/Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuNavigation"

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

								return FText::FormatNamed(LOCTEXT("TrajectoryFuelFormat", "{used}T of propellant will be used ({remaining}T remaining)"),
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
				]
			]
		]

		// Hover text slot
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride(500)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.Padding(Theme.ContentPadding)
				[
					SNew(STextBlock)
					.Text(this, &SNovaMainMenuNavigation::GetHoverText)
					.TextStyle(&Theme.MainFont)
				]
			]
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

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void SNovaMainMenuNavigation::UpdateSidePanel()
{
	bool HasValidDestination = false;

	for (const FNovaOrbitalObject& Object : SelectedObjectList)
	{
		// Valid area found
		if (Object.Area.IsValid() && SelectDestination(Object.Area.Get()))
		{
			HasValidDestination = true;
			AreaTitle->SetText(Object.Area->Name);
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
			const FNovaSpacecraft* Spacecraft = GameState->GetSpacecraft(Identifier);
			if (Spacecraft == nullptr || !Spacecraft->IsValid())
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

FText SNovaMainMenuNavigation::GetHoverText() const
{
	auto GetText = [](const FNovaOrbitalObject& Object, const ANovaGameState* GameState)
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

			return FText::FormatNamed(LOCTEXT("ManeuverFormat", "{duration} burn for a {deltav} m/s maneuver at {phase}° in {time}"),
				TEXT("phase"), FText::AsNumber(FMath::Fmod(Object.Maneuver->Phase, 360.0f), &NumberOptions), TEXT("time"),
				GetDurationText(Object.Maneuver->Time - GameState->GetCurrentTime()), TEXT("duration"),
				GetDurationText(Object.Maneuver->Duration), TEXT("deltav"), FText::AsNumber(Object.Maneuver->DeltaV, &NumberOptions));
		}
		else
		{
			return FText();
		}
	};

	// Build text for all objects
	FString Result;
	if (IsValid(GameState))
	{
		for (const FNovaOrbitalObject& Object : OrbitalMap->GetHoveredOrbitalObjects())
		{
			if (!Result.IsEmpty())
			{
				Result += "\n";
			}

			Result += GetText(Object, GameState).ToString();
		}
	}

	if (Result.IsEmpty())
	{
		return LOCTEXT("HoverTextHint", "Hover points of interest to learn more");
	}
	else
	{
		return FText::FromString(Result);
	}
}

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
