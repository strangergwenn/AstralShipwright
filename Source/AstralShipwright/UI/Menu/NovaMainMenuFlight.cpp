// Astral Shipwright - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Game/NovaArea.h"
#include "Game/NovaAsteroid.h"
#include "Game/NovaGameMode.h"
#include "Game/NovaGameState.h"
#include "Game/NovaOrbitalSimulationComponent.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "System/NovaMenuManager.h"
#include "Player/NovaPlayerController.h"

#include "UI/Widget/NovaButton.h"
#include "UI/Widget/NovaFadingWidget.h"
#include "UI/Widget/NovaKeyLabel.h"

#include "Nova.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    HUD panel stack
----------------------------------------------------*/

DECLARE_DELEGATE_OneParam(FNovaHUDUpdate, int32);

class SNovaHUDPanel : public SNovaFadingWidget<false>
{
	SLATE_BEGIN_ARGS(SNovaHUDPanel)
	{}

	SLATE_ARGUMENT(FNovaHUDUpdate, OnUpdate)
	SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

public:

	SNovaHUDPanel() : DesiredIndex(-1), CurrentIndex(-1)
	{}

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

	void SetHUDIndex(int32 Index)
	{
		DesiredIndex = Index;
	}

protected:

	virtual bool IsDirty() const override
	{
		if (CurrentIndex != DesiredIndex)
		{
			return true;
		}

		return false;
	}

	virtual void OnUpdate() override
	{
		CurrentIndex = DesiredIndex;

		if (UpdateCallback.IsBound())
		{
			UpdateCallback.Execute(CurrentIndex);
		}
	}

protected:

	FNovaHUDUpdate UpdateCallback;
	int32          DesiredIndex;
	int32          CurrentIndex;
};

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

SNovaMainMenuFlight::SNovaMainMenuFlight()
	: PC(nullptr), SpacecraftPawn(nullptr), SpacecraftMovement(nullptr), GameState(nullptr), OrbitalSimulation(nullptr)
{}

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Invisible widget
	SAssignNew(InvisibleWidget, SBox);

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	[
		SNew(SVerticalBox)

		// HUD content
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBackgroundBlur)
			.BlurRadius(TAttribute<TOptional<int32>>::Create(TAttribute<TOptional<int32>>::FGetter::CreateLambda([=]()
			{
				return CurrentAlpha * HUDPanel->GetBlurRadius().Get(0);
			})))
			.BlurStrength(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([=]()
			{
				return CurrentAlpha * HUDPanel->GetBlurStrength();
			})))
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SAssignNew(HUDPanel, SNovaHUDPanel)
				.OnUpdate(FNovaHUDUpdate::CreateSP(this, &SNovaMainMenuFlight::SetHUDIndexCallback))
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuBackground)
					.Padding(0)
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBackground)
						.Padding(0)
						[
							SAssignNew(HUDBox, SBox)
						]
					]
				]
			]
		]

		// HUD switcher
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBackgroundBlur)
			.BlurRadius(this, &SNovaTabPanel::GetBlurRadius)
			.BlurStrength(this, &SNovaTabPanel::GetBlurStrength)
			.bApplyAlphaToBlur(true)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(&Theme.MainMenuBackground)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuGenericBorder)
					.Padding(FMargin(Theme.ContentPadding.Left, 0, Theme.ContentPadding.Right, 0))
					[
						SAssignNew(HUDSwitcher, SHorizontalBox)
					]
				]
			]
		]
	];

	// Layout our data
	FNovaHUDData& PowerHUD = HUDData[0];
	FNovaHUDData& AttitudeHUD = HUDData[1];
	FNovaHUDData& HomeHUD = HUDData[2];
	FNovaHUDData& OperationsHUD = HUDData[3];
	FNovaHUDData& WeaponsHUD = HUDData[4];
	
	/*----------------------------------------------------
	    Power
	----------------------------------------------------*/

	SAssignNew(PowerHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Power", "Power"));

	SAssignNew(PowerHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("Power", "Power"))
		];

	PowerHUD.DefaultFocus = nullptr;
	
	/*----------------------------------------------------
	    Attitude
	----------------------------------------------------*/

	SAssignNew(AttitudeHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Propulsion", "Propulsion"));

	SAssignNew(AttitudeHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("Propulsion", "Propulsion"))
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Action(FNovaPlayerInput::MenuSecondary)
			.Text(LOCTEXT("ToggleOrbiting", "Toggle orbiting"))
			.OnClicked(FSimpleDelegate::CreateLambda([=]
			{
				if (!SpacecraftMovement->IsOrbiting())
				{
					SpacecraftMovement->StartOrbiting();
				}
				else
				{
					SpacecraftMovement->StopOrbiting();
				}
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				TArray<AActor*> Asteroids;
				UGameplayStatics::GetAllActorsOfClass(PC->GetWorld(), ANovaAsteroid::StaticClass(), Asteroids);
				return Asteroids.Num() > 0;
			})))
		]
		
		// Dock, undock, anchor, leave anchor
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaAssignNew(DockButton, SNovaButton)
			.Action(FNovaPlayerInput::MenuPrimary)
			.Text(this, &SNovaMainMenuFlight::GetDockUndockText)
			.HelpText(this, &SNovaMainMenuFlight::GetDockUndockHelp)
			.OnClicked(this, &SNovaMainMenuFlight::OnDockUndock)
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return CanDockUndock();
			})))
		];

	AttitudeHUD.DefaultFocus = DockButton;
	
	/*----------------------------------------------------
	    Home
	----------------------------------------------------*/

	SAssignNew(HomeHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.InfoFont)
		.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([=]()
		{
			return SpacecraftPawn ? SpacecraftPawn->GetSpacecraftCopy().GetName() : FText();
		})));

	SAssignNew(HomeHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaAssignNew(FastForwardButton, SNovaButton)
			.Action(FNovaPlayerInput::MenuPrimary)
			.Icon(FNovaStyleSet::GetBrush("Icon/SB_Time"))
			.Text(LOCTEXT("FastForward", "Fast forward"))
			.HelpText(this, &SNovaMainMenuFlight::GetFastFowardHelp)
			.OnClicked(this, &SNovaMainMenuFlight::OnFastForward)
			.Enabled(this, &SNovaMainMenuFlight::CanFastForward)
		];

	HomeHUD.DefaultFocus = FastForwardButton;

	/*----------------------------------------------------
	    Operations
	----------------------------------------------------*/

	SAssignNew(OperationsHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Operations", "Operations"));

	SAssignNew(OperationsHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("Operations", "Operations"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaAssignNew(TerminateButton, SNovaButton)
			.Text(LOCTEXT("AbortFlightPlan", "Terminate flight plan"))
			.HelpText(LOCTEXT("AbortTrajectoryHelp", "Terminate the current flight plan and stay on the current orbit"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				OrbitalSimulation->AbortTrajectory(GameState->GetPlayerSpacecraftIdentifiers());
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return OrbitalSimulation && GameState && OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier());
			})))
		]
		
		/*+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Action(FNovaPlayerInput::MenuSecondary)
			.Text(LOCTEXT("TestSilentRunning", "Silent running"))
			.OnClicked(FSimpleDelegate::CreateLambda([&]()
			{
				MenuManager->SetInterfaceColor(Theme.NegativeColor, FLinearColor(1.0f, 0.0f, 0.1f));
			}))
		]*/
		
#if WITH_EDITOR && 1

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TimeDilation", "Disable time dilation"))
			.HelpText(LOCTEXT("TimeDilationHelp", "Set time dilation to zero"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Normal);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Normal);
			})))
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TimeDilation1", "Time dilation 1"))
			.HelpText(LOCTEXT("TimeDilation1Help", "Set time dilation to 1 (1s = 1m)"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Level1);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level1);
			})))
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TimeDilation2", "Time dilation 2"))
			.HelpText(LOCTEXT("TimeDilation2Help", "Set time dilation to 2 (1s = 20m)"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Level2);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
			{
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level2);
			})))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNovaNew(SNovaButton)
			.Text(LOCTEXT("TestJoin", "Join random session"))
			.HelpText(LOCTEXT("TestJoinHelp", "Join random session"))
			.OnClicked(FSimpleDelegate::CreateLambda([&]()
			{
				PC->TestJoin();
			}))
		]
#endif // WITH_EDITOR
		;

	OperationsHUD.DefaultFocus = TerminateButton;

	/*----------------------------------------------------
	    Weapons
	----------------------------------------------------*/

	SAssignNew(WeaponsHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.MainFont)
		.Text(LOCTEXT("Weapons", "Weapons"));

	SAssignNew(WeaponsHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.HeadingFont)
			.Text(LOCTEXT("Weapons", "Weapons"))
		];

	WeaponsHUD.DefaultFocus = nullptr;
	
	/*----------------------------------------------------
	    HUD panel construction
	----------------------------------------------------*/
	
	// Previous key
	HUDSwitcher->AddSlot()
	.AutoWidth()
	[
		SNew(SNovaKeyLabel)
		.Key(this, &SNovaMainMenuFlight::GetPreviousItemKey)
	];

	// Build HUD selection entries
	for (int32 HUDIndex = 0; HUDIndex < HUDCount; HUDIndex++)
	{
		HUDSwitcher->AddSlot()
		.AutoWidth()
		[
			SNew(SNovaButton) // No navigation
			.Focusable(false)
			.Theme("TabButton")
			.Size("HUDButtonSize")
			.HelpText(HUDData[HUDIndex].HelpText)
			.OnClicked(this, &SNovaMainMenuFlight::SetHUDIndex, HUDIndex)
			.UserSizeCallback(FNovaButtonUserSizeCallback::CreateLambda([=]
			{
				return HUDAnimation.GetAlpha(HUDIndex);
			}))
			.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([=]()
			{
				return HUDIndex != CurrentHUDIndex;
			})))
			.Content()
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					HUDData[HUDIndex].OverviewWidget.ToSharedRef()
				]
			]
		];
	}
	
	// Next key
	HUDSwitcher->AddSlot()
	.AutoWidth()
	[
		SNew(SNovaKeyLabel)
		.Key(this, &SNovaMainMenuFlight::GetNextItemKey)
	];

	// clang-format on

	// Initialize
	HUDAnimation.Initialize(FNovaStyleSet::GetButtonTheme().AnimationDuration);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNovaTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Reset the HUD at center
	if (MenuManager.IsValid() && !MenuManager->IsIdle() && CurrentHUDIndex != DefaultHUDIndex)
	{
		SetHUDIndex(DefaultHUDIndex);
	}

	HUDAnimation.Update(CurrentHUDIndex, DeltaTime);
}

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();

	// Start the HUD at center
	SetHUDIndex(DefaultHUDIndex);
	HUDPanel->Reset();
	HUDAnimation.Update(CurrentHUDIndex, FNovaStyleSet::GetButtonTheme().AnimationDuration);
}

void SNovaMainMenuFlight::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuFlight::UpdateGameObjects()
{
	PC                 = MenuManager.IsValid() ? MenuManager->GetPC() : nullptr;
	SpacecraftPawn     = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	SpacecraftMovement = IsValid(SpacecraftPawn) ? SpacecraftPawn->GetSpacecraftMovement() : nullptr;
	GameState          = IsValid(PC) ? PC->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
}

void SNovaMainMenuFlight::Next()
{
	SetHUDIndex(FMath::Min(CurrentHUDIndex + 1, HUDCount - 1));
}

void SNovaMainMenuFlight::Previous()
{
	SetHUDIndex(FMath::Max(CurrentHUDIndex - 1, 0));
}

void SNovaMainMenuFlight::ZoomIn()
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->ZoomIn();
	}
}

void SNovaMainMenuFlight::ZoomOut()
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->ZoomOut();
	}
}

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (IsValid(SpacecraftPawn))
	{
		SpacecraftPawn->TiltInput(Value);
	}
}

void SNovaMainMenuFlight::OnClicked(const FVector2D& Position)
{
	SetHUDIndex(DefaultHUDIndex);
}

TSharedPtr<SNovaButton> SNovaMainMenuFlight::GetDefaultFocusButton() const
{
	return HUDData[CurrentHUDIndex].DefaultFocus;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

FText SNovaMainMenuFlight::GetDockUndockText() const
{
	if (IsInSpace())
	{
		if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Anchored)
		{
			return LOCTEXT("LeaveAnchor", "Leave anchor");
		}
		else
		{
			return LOCTEXT("Anchor", "Anchor");
		}
	}
	else
	{
		if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Docked)
		{
			return LOCTEXT("Undock", "Undock");
		}
		else
		{
			return LOCTEXT("Dock", "Dock");
		}
	}
}

FText SNovaMainMenuFlight::GetDockUndockHelp() const
{
	FText Out;

	if (IsInSpace())
	{
		if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Anchored)
		{
			Out = LOCTEXT("LeaveAnchorHelp", "Leave the current anchor and go back to orbiting the object");
			CanDockUndock(&Out);
		}
		else
		{
			Out = LOCTEXT("AnchorHelp", "Anchor to the currently orbited object");
			CanDockUndock(&Out);
		}
	}
	else
	{
		if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Docked)
		{
			Out = LOCTEXT("UndockHelp", "Undock from the station");
			CanDockUndock(&Out);
		}
		else
		{
			Out = LOCTEXT("DockHelp", "Dock at the station");
			CanDockUndock(&Out);
		}
	}

	return Out;
}

bool SNovaMainMenuFlight::CanDockUndock(FText* Help) const
{
	if (OrbitalSimulation && IsValid(PC) && IsValid(SpacecraftMovement) && IsValid(SpacecraftPawn))
	{
		// Anchoring
		if (IsInSpace())
		{
			// Leaving anchor
			if (SpacecraftMovement->GetState() == ENovaMovementState::Anchored)
			{
				// TODO
			}

			// Anchoring
			else
			{
				TArray<AActor*> Asteroids;
				UGameplayStatics::GetAllActorsOfClass(GameState->GetWorld(), ANovaAsteroid::StaticClass(), Asteroids);

				if (Asteroids.Num() == 0)
				{
					if (Help)
					{
						*Help = LOCTEXT("NoAsteroids", "No asteroid to anchor to");
					}
					return false;
				}
				else if (!IsValid(SpacecraftPawn->GetAnchorComponent()))
				{
					if (Help)
					{
						*Help = LOCTEXT("NoAnchorPoint", "This spacecraft cannot anchor to asteroids");
					}
					return false;
				}
			}
		}

		// Docking
		else
		{
			// Undocking
			if (SpacecraftMovement->GetState() == ENovaMovementState::Docked)
			{
				if (SpacecraftPawn->HasModifications())
				{
					if (Help)
					{
						*Help = LOCTEXT("SpacecraftModifications", "Cannot undock with design changes");
					}
					return false;
				}
				else if (PC->GetSpacecraft()->PropellantMassAtLaunch <= 0)
				{
					if (Help)
					{
						*Help = LOCTEXT("SpacecraftNoPropellant", "Cannot undock without propellant");
					}
					return false;
				}
				else if (!SpacecraftPawn->IsSpacecraftValid())
				{
					if (Help)
					{
						*Help = LOCTEXT("SpacecraftNotValid", "Cannot undock with an invalid spacecraft design");
					}
					return false;
				}
				else if (!SpacecraftMovement->CanUndock())
				{
					if (Help)
					{
						*Help = LOCTEXT("SpacecraftCannotUndock", "UndockingDenied");
					}
					return false;
				}
			}

			// Docking
			else
			{
				const FNovaTrajectory* PlayerTrajectory = OrbitalSimulation->GetPlayerTrajectory();
				if (PlayerTrajectory != nullptr)
				{
					if (Help)
					{
						*Help = LOCTEXT("SpacecraftTrajectory", "Cannot dock while on a trajectory");
					}
					return false;
				}
				else if (!SpacecraftMovement->CanDock())
				{
					if (Help)
					{
						*Help = LOCTEXT("SpacecraftCannotDock", "Docking denied");
					}
					return false;
				}
			}
		}

		return true;
	}

	return false;
}

FText SNovaMainMenuFlight::GetFastFowardHelp() const
{
	FText HelpText = LOCTEXT("FastForwardHelp", "Wait until the next event");

	const ANovaGameMode* GameMode = MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>();
	if (GameMode)
	{
		GameMode->CanFastForward(&HelpText);
	}

	return HelpText;
}

bool SNovaMainMenuFlight::CanFastForward() const
{
	const ANovaGameMode* GameMode = MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>();
	return IsValid(GameMode) && GameMode->CanFastForward();
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuFlight::SetHUDIndex(int32 Index)
{
	NCHECK(Index >= 0 && Index < HUDCount);
	CurrentHUDIndex = Index;
	HUDPanel->SetHUDIndex(CurrentHUDIndex);
}

void SNovaMainMenuFlight::SetHUDIndexCallback(int32 Index)
{
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();

	// Apply the new widget
	TSharedPtr<SWidget>& NewWidget = HUDData[Index].DetailedWidget;
	if (NewWidget)
	{
		NewWidget->SetVisibility(EVisibility::Visible);
		HUDBox->SetContent(NewWidget.ToSharedRef());
		HUDBox->SetPadding(Theme.ContentPadding);
	}
	else
	{
		HUDBox->SetContent(InvisibleWidget.ToSharedRef());
		HUDBox->SetPadding(FMargin(0));
	}

	ResetNavigation();
}

void SNovaMainMenuFlight::OnFastForward()
{
	MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->FastForward();
}

void SNovaMainMenuFlight::OnDockUndock()
{
	if (CanDockUndock())
	{
		if (IsInSpace())
		{
			if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Anchored)
			{
				SpacecraftMovement->ExitAnchor();
			}
			else
			{
				SpacecraftMovement->Anchor();
			}
		}
		else
		{
			if (IsValid(SpacecraftMovement) && SpacecraftMovement->GetState() == ENovaMovementState::Docked)
			{
				PC->Undock();
			}
			else
			{
				PC->Dock();
			}
		}
	}
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

FKey SNovaMainMenuFlight::GetPreviousItemKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuPrevious);
}

FKey SNovaMainMenuFlight::GetNextItemKey() const
{
	return MenuManager->GetFirstActionKey(FNovaPlayerInput::MenuNext);
}

bool SNovaMainMenuFlight::IsInSpace() const
{
	return IsValid(GameState) && IsValid(GameState->GetCurrentArea()) && GameState->GetCurrentArea()->IsInSpace;
}

#undef LOCTEXT_NAMESPACE
