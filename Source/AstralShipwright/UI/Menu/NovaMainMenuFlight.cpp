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
#include "Spacecraft/System/NovaSpacecraftCrewSystem.h"
#include "Spacecraft/System/NovaSpacecraftProcessingSystem.h"
#include "Spacecraft/System/NovaSpacecraftPowerSystem.h"

#include "Player/NovaPlayerController.h"

#include "Nova.h"

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/UI/Widgets/NeutronButton.h"
#include "Neutron/UI/Widgets/NeutronFadingWidget.h"
#include "Neutron/UI/Widgets/NeutronKeyLabel.h"
#include "Neutron/UI/Widgets/NeutronModalPanel.h"

#include "Widgets/Layout/SBackgroundBlur.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    HUD panel stack
----------------------------------------------------*/

DECLARE_DELEGATE_OneParam(FNovaHUDUpdate, int32);

class SNovaHUDPanel : public SNeutronFadingWidget<false>
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
		SNeutronFadingWidget::Construct(SNeutronFadingWidget::FArguments().Content()
		[
			InArgs._Content.Widget
		]);
		// clang-format on

		UpdateCallback = InArgs._OnUpdate;
		SetColorAndOpacity(TAttribute<FLinearColor>(this, &SNeutronFadingWidget::GetLinearColor));
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
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();
	MenuManager                    = InArgs._MenuManager;

	// Parent constructor
	SNeutronNavigationPanel::Construct(SNeutronNavigationPanel::FArguments().Menu(InArgs._Menu));

	// Invisible widget
	SAssignNew(InvisibleWidget, SBox);

	// clang-format off
	ChildSlot
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	[
		SNew(SHorizontalBox)
		
		+ SHorizontalBox::Slot()
	
		// Power & crew
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(20, 0))
		[
			SNew(SNeutronButtonLayout)
			.Size("InfoButtonSize")
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNeutronTabPanel::GetBlurRadius)
				.BlurStrength(this, &SNeutronTabPanel::GetBlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuBackground)
					.Padding(0)
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBorder)
						.Padding(Theme.ContentPadding)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SNeutronRichText)
								.Text(FNeutronTextGetter::CreateRaw(this, &SNovaMainMenuFlight::GetCrewText))
								.TextStyle(&Theme.MainFont)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SRichTextBlock)
								.Text(this, &SNovaMainMenuFlight::GetPowerText)
								.TextStyle(&Theme.MainFont)
								.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
								+ SRichTextBlock::ImageDecorator()
							]
						]
					]
				]
			]
		]

		// Central HUD
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SVerticalBox)

			// HUD content
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBackgroundBlur)
				.BlurRadius_Lambda([=]()
				{
					return CurrentAlpha * HUDPanel->GetBlurRadius().Get(0);
				})
				.BlurStrength_Lambda([=]()
				{
					return CurrentAlpha * HUDPanel->GetBlurStrength();
				})
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
				.BlurRadius(this, &SNeutronTabPanel::GetBlurRadius)
				.BlurStrength(this, &SNeutronTabPanel::GetBlurStrength)
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
		]
	
		// Status text
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(20, 0))
		[
			SNew(SNeutronButtonLayout)
			.Size("InfoButtonSize")
			[
				SNew(SBackgroundBlur)
				.BlurRadius(this, &SNeutronTabPanel::GetBlurRadius)
				.BlurStrength(this, &SNeutronTabPanel::GetBlurStrength)
				.bApplyAlphaToBlur(true)
				.Padding(0)
				[
					SNew(SBorder)
					.BorderImage(&Theme.MainMenuBackground)
					.Padding(0)
					[
						SNew(SBorder)
						.BorderImage(&Theme.MainMenuGenericBorder)
						.Padding(Theme.ContentPadding)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							[
								SNew(SNeutronRichText)
								.Text(FNeutronTextGetter::CreateRaw(this, &SNovaMainMenuFlight::GetStatusText))
								.TextStyle(&Theme.MainFont)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(this, &SNovaMainMenuFlight::GetStatusValue)
								.TextStyle(&Theme.MainFont)
							]
						]
					]
				]
			]
		]

		+ SHorizontalBox::Slot()
	];

	// Layout our data
	FNovaHUDData& AttitudeHUD = HUDData[0];
	FNovaHUDData& HomeHUD = HUDData[1];
	FNovaHUDData& OperationsHUD = HUDData[2];
	
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
			SNeutronNew(SNeutronButton)
			.Text(LOCTEXT("RequestSalvageButton", "Request salvage"))
			.HelpText(LOCTEXT("RequestSalvageButtonHelp", "Request salvage operations to refill propellant"))
			.OnClicked(this, & SNovaMainMenuFlight::OnRequestSalvage)
			.Enabled_Lambda([=]()
			{
				return IsValid(SpacecraftMovement) && !SpacecraftMovement->IsDocked()
					&& OrbitalSimulation && GameState && !OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier());
			})
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronAssignNew(TerminateButton, SNeutronButton)
			.Text(LOCTEXT("AbortFlightPlan", "Terminate flight plan"))
			.HelpText(LOCTEXT("AbortTrajectoryHelp", "Terminate the current flight plan and stay on the current orbit"))
			.OnClicked(FSimpleDelegate::CreateLambda([this]()
			{
				OrbitalSimulation->AbortTrajectory(GameState->GetPlayerSpacecraftIdentifiers());
			}))
			.Enabled_Lambda([=]()
			{
				return OrbitalSimulation && GameState && OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier());
			})
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronNew(SNeutronButton)
			.Action(FNeutronPlayerInput::MenuSecondary)
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
			.Enabled(this, &SNovaMainMenuFlight::CanOrbit)
		]
		
		// Dock, undock, anchor, leave anchor
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronAssignNew(DockButton, SNeutronButton)
			.Action(FNeutronPlayerInput::MenuPrimary)
			.Text(this, &SNovaMainMenuFlight::GetDockUndockText)
			.HelpText(this, &SNovaMainMenuFlight::GetDockUndockHelp)
			.OnClicked(this, &SNovaMainMenuFlight::OnDockUndock)
			.Enabled_Lambda([=]()
			{
				return CanDockUndock();
			})
		];

	AttitudeHUD.DefaultFocus = DockButton;
	
	/*----------------------------------------------------
	    Home
	----------------------------------------------------*/

	SAssignNew(HomeHUD.OverviewWidget, STextBlock)
		.TextStyle(&Theme.InfoFont)
		.Text_Lambda([=]()
		{
			return SpacecraftPawn ? SpacecraftPawn->GetSpacecraftCopy().GetName() : FText();
		});

	SAssignNew(HomeHUD.DetailedWidget, SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronAssignNew(FastForwardButton, SNeutronButton)
			.Action(FNeutronPlayerInput::MenuPrimary)
			.Icon(FNeutronStyleSet::GetBrush("Icon/SB_Time"))
			.Text(LOCTEXT("FastForward", "Fast forward"))
			.HelpText(this, &SNovaMainMenuFlight::GetFastFowardHelp)
			.OnClicked(this, &SNovaMainMenuFlight::OnFastForward)
			.Enabled(this, &SNovaMainMenuFlight::CanFastForward)
		]
	
#if WITH_EDITOR && 1

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronNew(SNeutronButton)
			.Text(LOCTEXT("TimeDilation", "Disable time dilation"))
			.HelpText(LOCTEXT("TimeDilationHelp", "Set time dilation to zero"))
			.OnClicked_Lambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Normal);
			})
			.Enabled_Lambda([=]()
			{
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Normal);
			})
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronNew(SNeutronButton)
			.Text(LOCTEXT("TimeDilation1", "Time dilation 1"))
			.HelpText(LOCTEXT("TimeDilation1Help", "Set time dilation to 1 (1s = 1m)"))
			.OnClicked_Lambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Level1);
			})
			.Enabled_Lambda([=]()
			{
				const ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level1);
			})
		]
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronNew(SNeutronButton)
			.Text(LOCTEXT("TimeDilation2", "Time dilation 2"))
			.HelpText(LOCTEXT("TimeDilation2Help", "Set time dilation to 2 (1s = 20m)"))
			.OnClicked_Lambda([this]()
			{
				GameState->SetTimeDilation(ENovaTimeDilation::Level2);
			})
			.Enabled_Lambda([=]()
			{
				return GameState && GameState->CanDilateTime(ENovaTimeDilation::Level2);
			})
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNeutronNew(SNeutronButton)
			.Text(LOCTEXT("TestJoin", "Join random session"))
			.HelpText(LOCTEXT("TestJoinHelp", "Join random session"))
			.OnClicked(FSimpleDelegate::CreateLambda([&]()
			{
				PC->TestJoin();
			}))
		]
#endif // WITH_EDITOR
	;

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
			SNeutronAssignNew(MiningRigButton, SNeutronButton)
			.Toggle(true)
			.Text_Lambda([=]()
			{
				return ProcessingSystem && ProcessingSystem->IsMiningRigActive() && ProcessingSystem->CanMiningRigBeActive() ?
					LOCTEXT("StopMining", "Stop mining") : LOCTEXT("StartMining", "Start mining");
			})
			.HelpText_Lambda([=]()
			{
				FText Help;
				if (ProcessingSystem && ProcessingSystem->CanMiningRigBeActive(&Help))
				{
					return LOCTEXT("MiningHelp", "Toggle activity for this mining rig");
				}
				else
				{
					return Help;
				}
			})
			.Enabled_Lambda([=]()
			{
				if (ProcessingSystem && ProcessingSystem->GetMiningRigIndex() != INDEX_NONE)
				{
					auto Status = ProcessingSystem->GetMiningRigStatus();
					return Status == ENovaSpacecraftProcessingSystemStatus::Processing
						|| (Status == ENovaSpacecraftProcessingSystemStatus::Stopped && ProcessingSystem->CanMiningRigBeActive());
				}
				return false;
			})
			.OnClicked(FSimpleDelegate::CreateLambda([=]()
			{
				ProcessingSystem->SetMiningRigActive(MiningRigButton->IsActive());
			}))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(ModuleGroupsBox, SVerticalBox)
		];

	OperationsHUD.DefaultFocus = MiningRigButton;
	
	/*----------------------------------------------------
	    HUD panel construction
	----------------------------------------------------*/
	
	// Previous key
	HUDSwitcher->AddSlot()
	.AutoWidth()
	[
		SNew(SNeutronKeyLabel)
		.Action(FNeutronPlayerInput::MenuPrevious)
	];

	// Build HUD selection entries
	for (int32 HUDIndex = 0; HUDIndex < HUDCount; HUDIndex++)
	{
		HUDSwitcher->AddSlot()
		.AutoWidth()
		[
			SNew(SNeutronButton) // No navigation
			.Focusable(false)
			.Theme("TabButton")
			.Size("HUDButtonSize")
			.HelpText(HUDData[HUDIndex].HelpText)
			.OnClicked(this, &SNovaMainMenuFlight::SetHUDIndex, HUDIndex)
			.UserSizeCallback(FNeutronButtonUserSizeCallback::CreateLambda([=]
			{
				return HUDAnimation.GetAlpha(HUDIndex);
			}))
			.Enabled_Lambda([=]()
			{
				return HUDIndex != CurrentHUDIndex;
			})
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
		SNew(SNeutronKeyLabel)
		.Action(FNeutronPlayerInput::MenuNext)
	];

	// clang-format on

	GenericModalPanel = Menu->CreateModalPanel();

	// Initialize
	HUDAnimation.Initialize(FNeutronStyleSet::GetButtonTheme().AnimationDuration);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SNeutronTabPanel::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Reset the HUD at center
	if (MenuManager.IsValid() && !MenuManager->IsIdle() && CurrentHUDIndex != DefaultHUDIndex)
	{
		SetHUDIndex(DefaultHUDIndex);
	}

	HUDAnimation.Update(CurrentHUDIndex, DeltaTime);
}

void SNovaMainMenuFlight::Show()
{
	SNeutronTabPanel::Show();

	// Start the HUD at center
	SetHUDIndex(DefaultHUDIndex);
	HUDPanel->Reset();
	HUDAnimation.Update(CurrentHUDIndex, FNeutronStyleSet::GetButtonTheme().AnimationDuration);
}

void SNovaMainMenuFlight::Hide()
{
	SNeutronTabPanel::Hide();
}

void SNovaMainMenuFlight::UpdateGameObjects()
{
	PC                 = MenuManager.IsValid() ? MenuManager->GetPC<ANovaPlayerController>() : nullptr;
	Spacecraft         = IsValid(PC) ? PC->GetSpacecraft() : nullptr;
	SpacecraftPawn     = IsValid(PC) ? PC->GetSpacecraftPawn() : nullptr;
	SpacecraftMovement = IsValid(SpacecraftPawn) ? SpacecraftPawn->GetSpacecraftMovement() : nullptr;
	GameState          = IsValid(PC) ? PC->GetWorld()->GetGameState<ANovaGameState>() : nullptr;
	OrbitalSimulation  = IsValid(GameState) ? GameState->GetOrbitalSimulation() : nullptr;
	CrewSystem         = IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftCrewSystem>(Spacecraft) : nullptr;
	ProcessingSystem =
		IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftProcessingSystem>(Spacecraft) : nullptr;
	PowerSystem = IsValid(GameState) && Spacecraft ? GameState->GetSpacecraftSystem<UNovaSpacecraftPowerSystem>(Spacecraft) : nullptr;
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

TSharedPtr<SNeutronButton> SNovaMainMenuFlight::GetDefaultFocusButton() const
{
	return HUDData[CurrentHUDIndex].DefaultFocus;
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

FText SNovaMainMenuFlight::GetCrewText() const
{
	return CrewSystem ? FText::FormatNamed(INVTEXT("<img src=\"/Text/Crew\"/> {busy} / {total}"), TEXT("busy"),
							FText::AsNumber(CrewSystem->GetTotalBusyCrew()), TEXT("total"), FText::AsNumber(CrewSystem->GetCurrentCrew()))
	                  : FText();
}

FText SNovaMainMenuFlight::GetPowerText() const
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 1;
	Options.MaximumFractionalDigits = 1;

	return PowerSystem ? FText::FormatNamed(INVTEXT("<img src=\"/Text/Power\"/> {used} / {total} kWh"), TEXT("used"),
							 FText::AsNumber(PowerSystem->GetRemainingEnergy(), &Options), TEXT("total"),
							 FText::AsNumber(PowerSystem->GetEnergyCapacity(), &Options))
	                   : FText();
}

FText SNovaMainMenuFlight::GetStatusText() const
{
	const ANovaAsteroid* AsteroidActor =
		IsValid(SpacecraftPawn) ? UNeutronActorTools::GetClosestActor<ANovaAsteroid>(SpacecraftPawn, SpacecraftPawn->GetActorLocation())
								: nullptr;

	if (Spacecraft == nullptr || !Spacecraft->HasEquipment(UNovaRadioMastDescription::StaticClass()))
	{
		return LOCTEXT("NoSensors", "<img src=\"/Text/Sensor\"/> No sensor");
	}
	else if (IsInSpace() && IsValid(AsteroidActor) && IsValid(OrbitalSimulation) && IsValid(GameState) &&
			 !OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier()))
	{
		const FVector SpacecraftRelativeLocation =
			AsteroidActor->GetTransform().InverseTransformPosition(SpacecraftPawn->GetActorLocation());
		int32 Density = FMath::RoundToInt(100 * AsteroidActor->GetMineralDensity(SpacecraftRelativeLocation));

		return FText::FormatNamed(LOCTEXT("StatusTextFormatAsteroid", "<img src=\"/Text/Sensor\"/> Detecting {mineral}"), TEXT("mineral"),
			AsteroidActor->GetAsteroidData().MineralResource->Name);
	}
	else
	{
		return LOCTEXT("NoData", "<img src=\"/Text/Sensor\"/> No sensor activity");
	}
}

FText SNovaMainMenuFlight::GetStatusValue() const
{
	const ANovaAsteroid* AsteroidActor =
		IsValid(SpacecraftPawn) ? UNeutronActorTools::GetClosestActor<ANovaAsteroid>(SpacecraftPawn, SpacecraftPawn->GetActorLocation())
								: nullptr;

	if (IsInSpace() && IsValid(AsteroidActor) && IsValid(OrbitalSimulation) && IsValid(GameState) &&
		!OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier()))
	{
		const FVector SpacecraftRelativeLocation =
			AsteroidActor->GetTransform().InverseTransformPosition(SpacecraftPawn->GetActorLocation());
		int32 Density = FMath::RoundToInt(100 * AsteroidActor->GetMineralDensity(SpacecraftRelativeLocation));

		return FText::FormatNamed(LOCTEXT("StatusValueFormatAsteroid", "{density}%"), TEXT("density"), FText::AsNumber(Density));
	}
	else
	{
		return FText();
	}
}

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
				if (ProcessingSystem->GetMiningRigStatus() == ENovaSpacecraftProcessingSystemStatus::Processing)
				{
					if (Help)
					{
						*Help = LOCTEXT("Mining", "Mining rig is currently engaged");
					}
					return false;
				}
			}

			// Anchoring
			else
			{
				const ANovaAsteroid* AsteroidActor =
					UNeutronActorTools::GetClosestActor<ANovaAsteroid>(SpacecraftPawn, SpacecraftPawn->GetActorLocation());

				if (!IsValid(AsteroidActor))
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
				else if (OrbitalSimulation->IsOnTrajectory(GameState->GetPlayerSpacecraftIdentifier()))
				{
					if (Help)
					{
						*Help = LOCTEXT("IsOnTrajectory", "Cannot anchor while a flight plan is ongoing");
					}
					return false;
				}
				else if (SpacecraftMovement->GetState() != ENovaMovementState::Idle)
				{
					if (Help)
					{
						*Help = LOCTEXT("IsMovementBusy", "Cannot anchor under operations");
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

bool SNovaMainMenuFlight::CanOrbit() const
{
	if (IsValid(SpacecraftPawn))
	{
		const ANovaAsteroid* AsteroidActor =
			UNeutronActorTools::GetClosestActor<ANovaAsteroid>(SpacecraftPawn, SpacecraftPawn->GetActorLocation());

		return IsInSpace() && IsValid(AsteroidActor) && !OrbitalSimulation->IsOnStartedTrajectory(GameState->GetPlayerSpacecraftIdentifier());
	}
	else
	{
		return false;
	}
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
	const FNeutronMainTheme& Theme = FNeutronStyleSet::GetMainTheme();

	NCHECK(Spacecraft);
	if (SpacecraftPawn->IsDocked())
	{
		ProcessingSystem->Load(*Spacecraft);
		PowerSystem->Load(*Spacecraft);
		CrewSystem->Load(*Spacecraft);
	}

	// Add module groups
	ModuleGroupsBox->ClearChildren();
	for (int32 ProcessingGroupIndex = 0; ProcessingGroupIndex < ProcessingSystem->GetProcessingGroupCount(); ProcessingGroupIndex++)
	{
		const FNovaModuleGroup& Group = ProcessingSystem->GetModuleGroup(ProcessingGroupIndex);

		TSharedPtr<SHorizontalBox> StatusBox;

		// clang-format off
		ModuleGroupsBox->AddSlot()
		.AutoHeight()
		.Padding(Theme.VerticalContentPadding)
		[
			SNew(SBorder)
			.BorderImage(&Theme.MainMenuGenericBackground)
			.Padding(0)
			[
				SNew(SHorizontalBox)

				// Group icon & index
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(Theme.ContentPadding)
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.Padding(0)
					.BorderImage(new FSlateNoResource)
					.ColorAndOpacity(Group.Color)
					[
						SNew(SRichTextBlock)
						.TextStyle(&Theme.HeadingFont)
						.Text(FText::FormatNamed(INVTEXT("<img src=\"{icon}\"/>\n{index}"),
														TEXT("icon"), FNovaSpacecraft::GetModuleGroupIcon(Group.Type),
														TEXT("index"), FText::AsNumber(Group.Index + 1)))
						.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
						+ SRichTextBlock::ImageDecorator()
					]
				]
		
				// Crew status
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.AutoHeight()
					[
						SNew(SRichTextBlock)
						.TextStyle(&Theme.HeadingFont)
						.Text(INVTEXT("<img src=\"/Text/Crew\"/>"))
						.DecoratorStyleSet(&FNeutronStyleSet::GetStyle())
						+ SRichTextBlock::ImageDecorator()
					]

					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.AutoHeight()
					[
						SNew(SNeutronText)
						.TextStyle(&Theme.HeadingFont)
						.Text(FNeutronTextGetter::CreateLambda([=]() {
							return FText::FormatNamed(INVTEXT("{busy} / {total}"),
								TEXT("busy"), FText::AsNumber(CrewSystem->GetBusyCrew(ProcessingGroupIndex)),
								TEXT("total"), FText::AsNumber(CrewSystem->GetRequiredCrew(ProcessingGroupIndex)));
						}))
					]
				]
		
				// Production status
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(StatusBox, SHorizontalBox)
				]

				+ SHorizontalBox::Slot()

				// Processing group activity toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNeutronNew(SNeutronButton)
					.Size("HalfButtonSize")
					.Toggle(true)
					.Text_Lambda([=]()
					{
						return ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex) ? LOCTEXT("StopProcessing", "Stop") : LOCTEXT("StartProcessing", "Start");
					})
					.HelpText_Lambda([=]()
					{
						auto Status = ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex);
						bool IsActive = ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex);

						if (Status.Num() == 0)
						{
							return LOCTEXT("ProcessingNoneHelp", "This module group doesn't have processing modules");
						}
						else if (!IsActive && Status.Contains(ENovaSpacecraftProcessingSystemStatus::Docked))
						{
							return LOCTEXT("ProcessingDockedHelp", "Modules cannot be activated while docked");
						}
						else if (!IsActive && Status.Contains(ENovaSpacecraftProcessingSystemStatus::Blocked))
						{
							return LOCTEXT("ProcessingBlockedHelp", "This module group is blocked");
						}
						else
						{
							return LOCTEXT("ProcessingHelp", "Toggle activity for this module group");
						}
					})
					.Enabled_Lambda([=]()
					{
						auto Status = ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex);
						bool IsActive = ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex);

						return Status.Num() && (IsActive || (!Status.Contains(ENovaSpacecraftProcessingSystemStatus::Docked)
							&& !Status.Contains(ENovaSpacecraftProcessingSystemStatus::Blocked)));
					})
					.OnClicked(FSimpleDelegate::CreateLambda([=]()
					{
						ProcessingSystem->SetProcessingGroupActive(ProcessingGroupIndex, !ProcessingSystem->IsProcessingGroupActive(ProcessingGroupIndex));
					}))
				]
			]
		];
		// clang-format on

		// Add status icons
		for (int32 ChainIndex = 0; ChainIndex < ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex).Num(); ChainIndex++)
		{
			// clang-format off
			StatusBox->AddSlot()
			.AutoWidth()
			[
				SNew(SNeutronImage)
				.Image(FNeutronImageGetter::CreateLambda([=]()
				{
					switch (ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex)[ChainIndex])
					{
						default:
						case ENovaSpacecraftProcessingSystemStatus::Stopped:
						case ENovaSpacecraftProcessingSystemStatus::Docked:
						case ENovaSpacecraftProcessingSystemStatus::Blocked:
							return FNeutronStyleSet::GetBrush("Icon/SB_Warning");
						case ENovaSpacecraftProcessingSystemStatus::Processing:
							return FNeutronStyleSet::GetBrush("Icon/SB_On");
					}
				}))
				.ColorAndOpacity_Lambda([=]()
				{
					switch (ProcessingSystem->GetProcessingGroupStatus(ProcessingGroupIndex)[ChainIndex])
					{
						default:
						case ENovaSpacecraftProcessingSystemStatus::Stopped:
						case ENovaSpacecraftProcessingSystemStatus::Docked:
							return FLinearColor::White;
						case ENovaSpacecraftProcessingSystemStatus::Processing:
							return Theme.PositiveColor;
						case ENovaSpacecraftProcessingSystemStatus::Blocked:
							return Theme.NegativeColor;
					}
				})
			];
			// clang-format on
		}
	}

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

void SNovaMainMenuFlight::OnRequestSalvage()
{
	GenericModalPanel->Show(LOCTEXT("RequestSalvage", "Request salvage operation"),
		LOCTEXT("RequestSalvageHelp",
			"Do you need immediate assistance? A salvage tug will be dispatched to your location to fill your propellant tanks. A fee will "
			"be applied, and all cargo will be forfeit."),
		FSimpleDelegate::CreateLambda(
			[=]()
			{
				PC->SalvagePlayer();
			}));
}

/*----------------------------------------------------
    Helpers
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsInSpace() const
{
	return IsValid(GameState) && IsValid(GameState->GetCurrentArea()) && GameState->GetCurrentArea()->IsInSpace;
}

#undef LOCTEXT_NAMESPACE
