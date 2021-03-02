// Nova project - Gwennaël Arbona

#include "NovaMainMenuFlight.h"

#include "NovaMainMenu.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaArea.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

// TODO REMOVE
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameWorld.h"
#include "Nova/Game/NovaOrbitalSimulationComponent.h"

#include "Nova/Player/NovaMenuManager.h"

#include "Nova/Spacecraft/NovaSpacecraftPawn.h"
#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Player/NovaPlayerController.h"
#include "Nova/UI/Component/NovaOrbitalMap.h"
#include "Nova/Nova.h"

#include "Slate/SRetainerWidget.h"

#define LOCTEXT_NAMESPACE "SNovaMainMenuFlight"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

void SNovaMainMenuFlight::Construct(const FArguments& InArgs)
{
	// Data
	const FNovaMainTheme& Theme = FNovaStyleSet::GetMainTheme();
	MenuManager                 = InArgs._MenuManager;

	// Parent constructor
	SNovaNavigationPanel::Construct(SNovaNavigationPanel::FArguments().Menu(InArgs._Menu));

	// clang-format off
	ChildSlot
	[
		SNew(SHorizontalBox)

		// Main box
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Top)
		.AutoWidth()
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaNew(SNovaButton)
				.Text(LOCTEXT("TestJoin", "Join random session"))
				.HelpText(LOCTEXT("HelpTestJoin", "Join random session"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					MenuManager->GetPC()->TestJoin();
				}))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaNew(SNovaButton)
				.Text(LOCTEXT("LeaveStation", "Leave station"))
				.HelpText(LOCTEXT("HelpLeaveStation", "Leave station"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					const class UNovaArea* Orbit = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{D1D46588-494D-E081-ADE6-48AE0B010BBB}"));
					MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChangeArea(Orbit);
				}))
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
				{
					return MenuManager->GetPC() && MenuManager->GetPC()->GetLocalRole() == ROLE_Authority;
				})))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaNew(SNovaButton)
				.Text(LOCTEXT("GoToStation", "Go to station"))
				.HelpText(LOCTEXT("HelpGoToStation", "Go to station"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					const class UNovaArea* Station = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
					MenuManager->GetWorld()->GetAuthGameMode<ANovaGameMode>()->ChangeArea(Station);
				}))
				.Enabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([&]()
				{
					return MenuManager->GetPC() && MenuManager->GetPC()->GetLocalRole() == ROLE_Authority;
				})))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaAssignNew(UndockButton, SNovaButton)
				.Text(LOCTEXT("ComputeTrajectory", "Compute trajectory"))
				.HelpText(LOCTEXT("HelpComputeTrajectory", "Compute trajectory"))
				.OnClicked(FSimpleDelegate::CreateLambda([&]()
				{
					const class UNovaArea* StationA = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{3F74954E-44DD-EE5C-404A-FC8BF3410826}"));
					const class UNovaArea* StationB = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CCA2E0C7-43AE-CDD1-06CA-AF951F61C44A}"));
					const class UNovaArea* StationC = MenuManager->GetGameInstance()->GetCatalog()->GetAsset<UNovaArea>(FGuid("{CAC5C9B9-451B-1212-6EC4-E8918B69A795}"));
					
					ANovaGameState* GameState = MenuManager->GetWorld()->GetGameState<ANovaGameState>();
					NCHECK(GameState);
					ANovaGameWorld* GameWorld = GameState->GetGameWorld();
					NCHECK(GameWorld);
					UNovaOrbitalSimulationComponent* OrbitalSimulation = GameWorld->GetOrbitalSimulation();
					NCHECK(OrbitalSimulation);

					OrbitalMap->PreviewTrajectory(OrbitalSimulation->ComputeTrajectory(StationA, StationB));
				}))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNovaAssignNew(DockButton, SNovaButton)
				.Text(LOCTEXT("Dock", "Dock"))
				.HelpText(LOCTEXT("DockHelp", "Dock at the station"))
				.OnClicked(this, &SNovaMainMenuFlight::OnDock)
				.Enabled(this, &SNovaMainMenuFlight::IsDockEnabled)
			]
		]

		+ SHorizontalBox::Slot()
		[
			SAssignNew(MapRetainer, SRetainerWidget)
			[
				SAssignNew(OrbitalMap, SNovaOrbitalMap)
				.MenuManager(MenuManager)
			]
		]
	];
	// clang-format on

	// Setup retainer
	MapRetainer->SetTextureParameter(TEXT("UI"));
	MapRetainer->SetEffectMaterial(Theme.EffectMaterial);
}

/*----------------------------------------------------
    Interaction
----------------------------------------------------*/

void SNovaMainMenuFlight::Show()
{
	SNovaTabPanel::Show();

	GetSpacecraftPawn()->SetHighlightCompartment(INDEX_NONE);
}

void SNovaMainMenuFlight::Hide()
{
	SNovaTabPanel::Hide();
}

void SNovaMainMenuFlight::HorizontalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->PanInput(Value);
	}
}

void SNovaMainMenuFlight::VerticalAnalogInput(float Value)
{
	if (GetSpacecraftPawn())
	{
		GetSpacecraftPawn()->TiltInput(Value);
	}
}

TSharedPtr<SNovaButton> SNovaMainMenuFlight::GetDefaultFocusButton() const
{
	if (IsUndockEnabled())
	{
		return UndockButton;
	}
	else
	{
		return DockButton;
	}
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

ANovaSpacecraftPawn* SNovaMainMenuFlight::GetSpacecraftPawn() const
{
	return MenuManager->GetPC() ? MenuManager->GetPC()->GetSpacecraftPawn() : nullptr;
}

UNovaSpacecraftMovementComponent* SNovaMainMenuFlight::GetSpacecraftMovement() const
{
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	if (SpacecraftPawn)
	{
		return Cast<UNovaSpacecraftMovementComponent>(SpacecraftPawn->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
	}
	else
	{
		return nullptr;
	}
}

/*----------------------------------------------------
    Content callbacks
----------------------------------------------------*/

bool SNovaMainMenuFlight::IsUndockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->GetState() == ENovaMovementState::Docked;
}

bool SNovaMainMenuFlight::IsDockEnabled() const
{
	return GetSpacecraftMovement() && GetSpacecraftMovement()->GetState() == ENovaMovementState::Idle;
}

/*----------------------------------------------------
    Callbacks
----------------------------------------------------*/

void SNovaMainMenuFlight::OnUndock()
{
	NCHECK(IsUndockEnabled());

	MenuManager->GetPC()->Undock();
}

void SNovaMainMenuFlight::OnDock()
{
	NCHECK(IsDockEnabled());

	MenuManager->GetPC()->Dock();
}

#undef LOCTEXT_NAMESPACE
