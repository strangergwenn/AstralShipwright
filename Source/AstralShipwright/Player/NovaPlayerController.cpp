// Astral Shipwright - Gwennaël Arbona

#include "NovaPlayerController.h"
#include "NovaPlayerState.h"

#include "Game/NovaAsteroid.h"
#include "Game/NovaGameMode.h"
#include "Game/NovaGameState.h"
#include "Game/NovaPlanetarium.h"
#include "Game/NovaPlayerStart.h"
#include "Game/NovaSaveData.h"
#include "Game/Contract/NovaContract.h"
#include "Game/Station/NovaStationDock.h"

#include "Spacecraft/NovaSpacecraftPawn.h"
#include "Spacecraft/NovaSpacecraftDriveComponent.h"
#include "Spacecraft/NovaSpacecraftThrusterComponent.h"
#include "Spacecraft/NovaSpacecraftMovementComponent.h"

#include "UI/Menu/NovaMainMenu.h"
#include "UI/Menu/NovaOverlay.h"
#include "Nova.h"

#include "Neutron/Actor/NeutronActorTools.h"
#include "Neutron/Actor/NeutronTurntablePawn.h"
#include "Neutron/Player/NeutronGameViewportClient.h"
#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "Neutron/Settings/NeutronWorldSettings.h"
#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronContractManager.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/System/NeutronPostProcessManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronSaveManager.h"
#include "Neutron/System/NeutronSessionsManager.h"
#include "Neutron/System/NeutronSoundManager.h"
#include "Neutron/UI/Widgets/NeutronMenu.h"

#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"

#include "GameFramework/PlayerState.h"
#include "Components/PostProcessComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkyLightComponent.h"

#include "Engine/LocalPlayer.h"
#include "Engine/SpotLight.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

#include <inttypes.h>

#define LOCTEXT_NAMESPACE "ANovaPlayerController"

/*----------------------------------------------------
    Constructors
----------------------------------------------------*/

ANovaPlayerViewpoint::ANovaPlayerViewpoint() : Super()
{
	RootComponent = CreateDefaultSubobject<UCameraComponent>("Root");

	// Defaults
	CameraAnimationDuration = 5.0f;
	CameraPanAmount         = 5.0f;
	CameraTiltAmount        = 0.0f;
	CameraTravelingAmount   = 250.0f;
}

ANovaPlayerController::ANovaPlayerController()
	: Super()
	, CurrentCameraState(ENovaPlayerCameraState::Default)
	, CurrentTimeInCameraState(0)
	, PhotoModeAction(NAME_None)
	, SharedTransitionActive(false)
{}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

FNovaPlayerSave ANovaPlayerController::Save() const
{
	FNovaPlayerSave SaveData;

	// Save the spacecraft
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	NCHECK(SpacecraftPawn);
	const FNovaSpacecraft* Spacecraft = GetSpacecraft();
	if (Spacecraft)
	{
		SaveData.Spacecraft = *Spacecraft;
	}

	// Save credits
	SaveData.Credits = Credits;

	return SaveData;
}

void ANovaPlayerController::Load(const FNovaPlayerSave& SaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::Load");

	// Store the save data so that the spacecraft pawn can fetch it later when it spawns
	FNovaSpacecraft NewSpacecraft = SaveData.Spacecraft;
	NewSpacecraft.UpdatePropulsionMetrics();
	if (!NewSpacecraft.IsValid())
	{
		NewSpacecraft.Create(LOCTEXT("UnnamedSpacecraft", "Unnamed Spacecraft").ToString());
	}
	UpdateSpacecraft(NewSpacecraft);

	// Load credits
	Credits = SaveData.Credits;
	if (Credits == 0)
	{
#if WITH_EDITOR
		Credits = 20000;
#else
		Credits = 2000;
#endif    // WITH_EDITOR
	}
}

void ANovaPlayerController::SaveGame()
{
	NLOG("ANovaPlayerController::SaveGame");

	TSharedPtr<FNovaGameSave> SaveData = MakeShared<FNovaGameSave>();

	// Save ourselves
	SaveData->PlayerData = Save();

	// Save the world
	if (GetLocalRole() == ROLE_Authority)
	{
		ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		if (IsValid(GameState))
		{
			SaveData->GameStateData = GameState->Save();
		}
	}

	// Save contracts
	SaveData->ContractManagerData = UNeutronContractManager::Get()->Save();

	// Write the save data to the already open slot
	UNeutronSaveManager::Get()->SetCurrentSaveData<FNovaGameSave>(SaveData);
	UNeutronSaveManager::Get()->WriteCurrentSaveData<FNovaGameSave>();
	Notify(LOCTEXT("SavedGame", "Game saved"), FText(), ENeutronNotificationType::Save);
}

void ANovaPlayerController::LoadGame(const FString SaveName)
{
	NLOG("ANovaPlayerController::LoadGame");

	UNeutronSaveManager* SaveManager = UNeutronSaveManager::Get();
	NCHECK(SaveManager);

	// Load the save data from slot
	SaveManager->LoadGame<FNovaGameSave>(SaveName);
	NCHECK(SaveManager->HasLoadedSaveData());
	TSharedPtr<FNovaGameSave> SaveData = SaveManager->GetCurrentSaveData<FNovaGameSave>();

	// Load contracts
	UNeutronContractManager::Get()->Load(SaveData->ContractManagerData);
}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void ANovaPlayerController::BeginPlay()
{
	NLOG("ANovaPlayerController::BeginPlay");

	Super::BeginPlay();

	// Process client-side player initialization
	if (IsLocalPlayerController())
	{
		// Setup menus
		UNeutronMenuManager::Get()->BeginPlay<SNovaMainMenu, SNovaOverlay>(this);

		// Setup post-processing
		TSharedPtr<FNeutronPostProcessSetting> DefaultSettings = MakeShared<FNeutronPostProcessSetting>();
		UNeutronPostProcessManager::Get()->RegisterPreset(ENovaPostProcessPreset::Neutral, DefaultSettings);
		UNeutronPostProcessManager::Get()->BeginPlay(this,

			// Preset control
			FNeutronPostProcessControl::CreateWeakLambda(this,
				[=]()
				{
					ENovaPostProcessPreset TargetPreset = ENovaPostProcessPreset::Neutral;

					return static_cast<int32>(TargetPreset);
				}),

			// Preset tick
			FNeutronPostProcessUpdate::CreateWeakLambda(this,
				[=](UPostProcessComponent* Volume, UMaterialInstanceDynamic* Material,
					const TSharedPtr<FNeutronPostProcessSettingBase>& Current, const TSharedPtr<FNeutronPostProcessSettingBase>& Target,
					float Alpha)
				{
					UNeutronGameUserSettings*         GameUserSettings = Cast<UNeutronGameUserSettings>(GEngine->GetGameUserSettings());
					const FNeutronPostProcessSetting* MyCurrent        = static_cast<const FNeutronPostProcessSetting*>(Current.Get());
					const FNeutronPostProcessSetting* MyTarget         = static_cast<const FNeutronPostProcessSetting*>(Target.Get());

					// Custom settings
					ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
					Material->SetScalarParameterValue("HighlightAlpha", IsValid(SpacecraftPawn) ? SpacecraftPawn->GetHighlightAlpha() : 0);
					Material->SetScalarParameterValue("OutlineAlpha", IsValid(SpacecraftPawn) ? SpacecraftPawn->GetOutlineAlpha() : 0);
					Material->SetVectorParameterValue("HighlightColor", UNeutronMenuManager::Get()->GetHighlightColor());
					// Material->SetScalarParameterValue("ChromaIntensity", FMath::Lerp(Current.ChromaIntensity, Target.ChromaIntensity,
			        // Alpha));

					// Built-in settings (overrides)
					Volume->Settings.bOverride_FilmGrainIntensity = true;
					Volume->Settings.bOverride_SceneColorTint     = true;

					// Built in settings (values)
					Volume->Settings.FilmGrainIntensity = FMath::Lerp(MyCurrent->GrainIntensity, MyTarget->GrainIntensity, Alpha);
					Volume->Settings.SceneColorTint     = FMath::Lerp(MyCurrent->SceneColorTint, MyTarget->SceneColorTint, Alpha);
				}));

		// Setup sound
		UNeutronSoundManager::Get()->BeginPlay(this,    //
			FNeutronMusicCallback::CreateWeakLambda(this,
				[this]()
				{
					if (IsOnMainMenu())
					{
						return "Menu";
					}
					else
					{
						return "Ambient";
					}
				}));

		// Setup thruster sounds
		UNeutronSoundManager::Get()->AddEnvironmentSound("Thrusters",    //
			FNeutronSoundInstanceCallback::CreateWeakLambda(this,
				[=]()
				{
					ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
					if (IsValid(SpacecraftPawn))
					{
						TArray<const UNovaSpacecraftThrusterComponent*> Thrusters;
						SpacecraftPawn->GetComponents(Thrusters);
						for (const UNovaSpacecraftThrusterComponent* Thruster : Thrusters)
						{
							if (Thruster->IsThrusterActive())
							{
								return true;
							}
						}
					}

					return false;
				}));

		// Setup drive sounds
		UNeutronSoundManager::Get()->AddEnvironmentSound("Drive",    //
			FNeutronSoundInstanceCallback::CreateWeakLambda(this,
				[=]()
				{
					ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
					if (IsValid(SpacecraftPawn))
					{
						TArray<const UNovaSpacecraftDriveComponent*> Drives;
						SpacecraftPawn->GetComponents(Drives);
						for (const UNovaSpacecraftDriveComponent* Drive : Drives)
						{
							if (Drive->IsDriveActive())
							{
								return true;
							}
						}
					}

					return false;
				}));

		// Setup contracts
		UNeutronContractManager::Get()->BeginPlay(this,    //
			FNeutronContractCreationCallback::CreateWeakLambda(this,
				[=](ENeutronContractType Type, UNeutronGameInstance* CurrentGameInstance)
				{
					// Create the contract
					TSharedPtr<FNeutronContract> Contract;
					if (Type == ENeutronContractType::Tutorial)
					{
						Contract = MakeShared<FNovaTutorialContract>();
					}
					else
					{
						NCHECK(false);
					}

					// Create the contract
					Contract->Initialize(CurrentGameInstance);

					return Contract;
				}));

		// Load save data, process local game startup
		if (!IsOnMainMenu())
		{
			ClientLoadPlayer();
		}
	}

	// Initialize persistent objects
	UNeutronSessionsManager* SessionsManager = UNeutronSessionsManager::Get();
	SessionsManager->SetAcceptedInvitationCallback(
		FNeutronOnFriendInviteAccepted::CreateUObject(this, &ANovaPlayerController::AcceptInvitation));

#if WITH_EDITOR

	// Start a host session if requested through command line
	if (FParse::Param(FCommandLine::Get(), TEXT("host")))
	{
		FCommandLine::Set(TEXT(""));

		SessionsManager->StartSession(ENovaConstants::DefaultLevel, ENovaConstants::MaxPlayerCount, true);
	}

#endif    // WITH_EDITOR
}

void ANovaPlayerController::PawnLeavingGame()
{
	NLOG("ANovaPlayerController::PawnLeavingGame");

	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	ANovaGameState*      GameState      = GetWorld()->GetGameState<ANovaGameState>();

	if (IsValid(SpacecraftPawn) && IsValid(GameState))
	{
		FGuid PlayerSpacecraftIdentifier = SpacecraftPawn->GetSpacecraftIdentifier();
		GameState->RemoveSpacecraft(PlayerSpacecraftIdentifier);
	}

	SetPawn(nullptr);
}

void ANovaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalPlayerController())
	{
		// Disable overlays in photo mode
		ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
		if (IsValid(SpacecraftPawn) && IsInPhotoMode())
		{
			SpacecraftPawn->SetOutlinedCompartment(INDEX_NONE);
			SpacecraftPawn->SetHighlightedCompartment(INDEX_NONE);
		}

		CurrentTimeInCameraState += DeltaTime;
	}
}

void ANovaPlayerController::GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const
{
	Super::GetPlayerViewPoint(Location, Rotation);

	// During cutscenes, use the closest camera viewpoint and focus the player ship
	if (IsReady() && (CurrentCameraState == ENovaPlayerCameraState::CinematicSpacecraft ||
						 CurrentCameraState == ENovaPlayerCameraState::CinematicEnvironment ||
						 CurrentCameraState == ENovaPlayerCameraState::CinematicBrake))
	{
		// Get points of interest nearby
		TArray<AActor*> Viewpoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlayerViewpoint::StaticClass(), Viewpoints);
		TArray<AActor*> Asteroids;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaAsteroid::StaticClass(), Asteroids);
		TArray<AActor*> StationDocks;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaStationDock::StaticClass(), StationDocks);
		TArray<AActor*> Planetariums;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlanetarium::StaticClass(), Planetariums);

		// Get player start
		ANovaSpacecraftPawn*    SpacecraftPawn = GetSpacecraftPawn();
		const ANovaPlayerStart* PlayerStart    = SpacecraftPawn->GetSpacecraftMovement()->GetPlayerStart();

		// Get the first viewpoint actor and extract its transform
		const ANovaPlayerViewpoint* PlayerViewpoint = nullptr;
		if (Viewpoints.Num())
		{
			UNeutronActorTools::SortActorsByClosestDistance(Viewpoints, GetPawn()->GetActorLocation());
			PlayerViewpoint = Cast<ANovaPlayerViewpoint>(Viewpoints[0]);
		}
		float AnimationDuration =
			PlayerViewpoint ? PlayerViewpoint->CameraAnimationDuration : GetDefault<ANovaPlayerViewpoint>()->CameraAnimationDuration;

		FVector PlayerLocation = GetPawn()->GetActorLocation();
		if (PlayerViewpoint)
		{
			Location = PlayerViewpoint->GetActorLocation();

			// Spacecraft focus
			if (CurrentCameraState == ENovaPlayerCameraState::CinematicSpacecraft)
			{
				Rotation = (PlayerLocation - Location).Rotation();
			}

			// Environment panning shot
			else if (CurrentCameraState == ENovaPlayerCameraState::CinematicEnvironment)
			{
				float AnimationAlpha = FMath::Clamp(CurrentTimeInCameraState / AnimationDuration, 0.0f, 1.0f);
				AnimationAlpha       = FMath::InterpEaseOut(-0.5f, 0.5f, AnimationAlpha, ENeutronUIConstants::EaseStandard);

				Rotation = PlayerViewpoint->GetActorRotation();
				Rotation +=
					FRotator(0, AnimationAlpha * PlayerViewpoint->CameraPanAmount, AnimationAlpha * PlayerViewpoint->CameraTiltAmount);
				Location += Rotation.Vector() * AnimationAlpha * PlayerViewpoint->CameraTravelingAmount;
			}
		}

		// Braking spacecraft entry
		if (CurrentCameraState == ENovaPlayerCameraState::CinematicBrake &&
			((StationDocks.Num() > 0 && IsValid(PlayerStart)) || Asteroids.Num() > 0))
		{
			NCHECK(Planetariums.Num() > 0);

			// Define scene parameters
			double        ViewDistance = StationDocks.Num() > 0 ? 20000.0 : 50000.0;
			const FVector TargetLocation =
				StationDocks.Num() > 0 ? PlayerStart->GetWaitingPointLocation() : Asteroids[0]->GetActorLocation();
			const FVector BackdropLocation =
				StationDocks.Num() > 0 ? PlayerStart->GetActorLocation() : Cast<ANovaPlanetarium>(Planetariums[0])->GetPlanetLocation();

			// Define the appropriate viewpoint characteristics
			const FVector TargetSeparation   = SpacecraftPawn->GetActorLocation() - TargetLocation;
			const FVector TargetViewpoint    = (SpacecraftPawn->GetActorLocation() + TargetLocation) / 2.0;
			const FVector ViewpointDirection = (BackdropLocation - TargetViewpoint).GetSafeNormal();
			ViewDistance                     = FMath::Max(ViewDistance, 2 * TargetSeparation.Size());

			Location = TargetViewpoint - ViewpointDirection * ViewDistance;
			Rotation = ViewpointDirection.Rotation();
		}
	}
}

bool ANovaPlayerController::IsReady() const
{
	if (IsOnMainMenu())
	{
		return Super::IsReady();
	}
	else
	{
		// Check spacecraft pawn
		ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
		bool IsSpacecraftReady = IsValid(SpacecraftPawn) && GetSpacecraft() && SpacecraftPawn->GetSpacecraftMovement()->IsInitialized();

		// Check game state
		ANovaGameState* GameState        = GetWorld()->GetGameState<ANovaGameState>();
		bool            IsGameStateReady = IsValid(GameState) && GameState->IsReady();

		return IsSpacecraftReady && IsGameStateReady;
	}
}

void ANovaPlayerController::Notify(const FText& Text, const FText& Subtext, ENeutronNotificationType Type)
{
	UNeutronMenuManager::Get()->GetOverlay<SNovaOverlay>()->Notify(Text, Subtext, Type);
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaPlayerController::ProcessTransaction(FNovaCredits CreditsDelta)
{
	// Authority
	if (GetLocalRole() == ROLE_Authority)
	{
		NCHECK(CanAffordTransaction(CreditsDelta));

		Credits += CreditsDelta;
		Credits = FMath::Max(Credits, FNovaCredits(0));

		NLOG("ANovaPlayerController::ProcessTransaction : %" PRId64 " in account (%+" PRId64 ")", Credits.GetValue(),
			CreditsDelta.GetValue());
	}

	// Remote client
	else if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerProcessTransaction(CreditsDelta);
	}
}

void ANovaPlayerController::ServerProcessTransaction_Implementation(FNovaCredits CreditsDelta)
{
	NCHECK(GetLocalRole() == ROLE_Authority);

	ProcessTransaction(CreditsDelta);
}

bool ANovaPlayerController::CanAffordTransaction(FNovaCredits CreditsDelta) const
{
	return Credits + CreditsDelta >= 0;
}

void ANovaPlayerController::Dock()
{
	NLOG("ANovaPlayerController::Dock");

	FSimpleDelegate EndCutscene = FSimpleDelegate::CreateLambda(
		[=]()
		{
			UNeutronMenuManager::Get()->OpenMenu(FNeutronAsyncAction::CreateLambda(
				[=]()
				{
					SetCameraState(ENovaPlayerCameraState::Default);
					GetSpacecraftPawn()->ResetView();
					SaveGame();
				}));
		});

	FNeutronAsyncAction StartCutscene = FNeutronAsyncAction::CreateLambda(
		[=]()
		{
			SetCameraState(ENovaPlayerCameraState::CinematicSpacecraft);
			GetSpacecraftPawn()->Dock(EndCutscene);
		});

	UNeutronMenuManager::Get()->CloseMenu(StartCutscene);
}

void ANovaPlayerController::Undock()
{
	NLOG("ANovaPlayerController::Undock");

	FSimpleDelegate EndCutscene = FSimpleDelegate::CreateLambda(
		[=]()
		{
			UNeutronMenuManager::Get()->OpenMenu(FNeutronAsyncAction::CreateLambda(
				[=]()
				{
					SetCameraState(ENovaPlayerCameraState::Default);
					GetSpacecraftPawn()->ResetView();
				}));
		});

	FNeutronAsyncAction StartCutscene = FNeutronAsyncAction::CreateLambda(
		[=]()
		{
			SaveGame();
			SetCameraState(ENovaPlayerCameraState::CinematicSpacecraft);
			GetSpacecraftPawn()->Undock(EndCutscene);
		});

	UNeutronMenuManager::Get()->CloseMenu(StartCutscene);
}

void ANovaPlayerController::SharedTransition(ENovaPlayerCameraState NewCameraState, FNeutronAsyncAction StartAction,
	FNeutronAsyncCondition Condition, FNeutronAsyncAction FinishAction)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::ServerSharedTransition");

	for (ANovaPlayerController* OtherPlayer : TActorRange<ANovaPlayerController>(GetWorld()))
	{
		OtherPlayer->ClientStartSharedTransition(NewCameraState);
	}

	SharedTransitionStartAction  = StartAction;
	SharedTransitionFinishAction = FinishAction;
	SharedTransitionCondition    = Condition;
}

void ANovaPlayerController::ClientStartSharedTransition_Implementation(ENovaPlayerCameraState NewCameraState)
{
	NLOG("ANovaPlayerController::ClientStartSharedTransition_Implementation");

	// Shared transitions work like this :
	// - Server signals all players to fade to black through this very method
	// - Once faded, Action is called and all players call ServerSharedTransitionReady() to signal they're dark
	// - Server fires SharedTransitionStartAction when all clients have called ServerSharedTransitionReady()
	// - Server fires SharedTransitionFinishAction once SharedTransitionCondition returns true on the server
	// - Server then calls ClientStopSharedTransition() on all players so that they know to resume
	// - All players then fade back to the game

	SharedTransitionActive = true;

	// Action : mark as in shared transition locally and remotely
	FNeutronAsyncAction Action = FNeutronAsyncAction::CreateLambda(
		[=]()
		{
			SetCameraState(NewCameraState);
			ServerSharedTransitionReady();
			NLOG("ANovaPlayerController::ClientStartSharedTransition_Implementation : done, waiting for server");
		});

	// Condition : on server, when all clients have reported as ready
	// On client, when the server has signaled to stop
	FNeutronAsyncCondition Condition = FNeutronAsyncCondition::CreateLambda(
		[=]()
		{
			if (GetLocalRole() == ROLE_Authority)
			{
				// Check if all players are in transition
				bool AllPlayersInTransition = true;
				for (ANovaPlayerController* OtherPlayer : TActorRange<ANovaPlayerController>(GetWorld()))
				{
					if (!OtherPlayer->SharedTransitionActive)
					{
						AllPlayersInTransition = false;
						break;
					}
				}

				// Once all players are in the transition, fire the start event, wait for the condition, fire the end event, and stop
				if (AllPlayersInTransition)
				{
					SharedTransitionStartAction.ExecuteIfBound();
					SharedTransitionStartAction.Unbind();

					if (!SharedTransitionCondition.IsBound() || SharedTransitionCondition.Execute())
					{
						SharedTransitionFinishAction.ExecuteIfBound();
						SharedTransitionFinishAction.Unbind();
						SharedTransitionCondition.Unbind();

						for (ANovaPlayerController* OtherPlayer : TActorRange<ANovaPlayerController>(GetWorld()))
						{
							OtherPlayer->ClientStopSharedTransition();
						}

						return true;
					}
				}

				return false;
			}
			else
			{
				return !SharedTransitionActive;
			}
		});

	// Run the process
	switch (NewCameraState)
	{
		// UI enabled states
		case ENovaPlayerCameraState::Default:
			UNeutronMenuManager::Get()->OpenMenu(Action, Condition);
			break;

		// UI disabled states
		case ENovaPlayerCameraState::CinematicSpacecraft:
		case ENovaPlayerCameraState::CinematicEnvironment:
		case ENovaPlayerCameraState::CinematicBrake:
		case ENovaPlayerCameraState::FastForward:
			UNeutronMenuManager::Get()->CloseMenu(Action, Condition);
			break;
	}
}

void ANovaPlayerController::ClientStopSharedTransition_Implementation()
{
	NLOG("ANovaPlayerController::ClientStopSharedTransition_Implementation");

	SharedTransitionActive = false;
}

void ANovaPlayerController::ServerSharedTransitionReady_Implementation()
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::ServerSharedTransitionReady_Implementation");

	SharedTransitionActive = true;
}

void ANovaPlayerController::SetCameraState(ENovaPlayerCameraState State)
{
	CurrentCameraState       = State;
	CurrentTimeInCameraState = 0;

	// Handle the fast-forward camera
	if (State == ENovaPlayerCameraState::FastForward)
	{
		UNeutronMenuManager::Get()->GetOverlay<SNovaOverlay>()->StartFastForward();
	}
	else
	{
		UNeutronMenuManager::Get()->GetOverlay<SNovaOverlay>()->StopFastForward();
	}
}

/*----------------------------------------------------
    Server-side save
----------------------------------------------------*/

void ANovaPlayerController::StartGame(FString SaveName, bool Online)
{
	NLOG("ANovaPlayerController::StartGame : loading from '%s', online = %d", *SaveName, Online);

	if (UNeutronMenuManager::Get()->IsIdle())
	{
		UNeutronSoundManager::Get()->Mute();

		UNeutronMenuManager::Get()->RunAction(ENeutronLoadingScreen::Launch,    //
			FNeutronAsyncAction::CreateLambda(
				[=]()
				{
					NLOG("UNovaGameInstance::StartGame");

					LoadGame(SaveName);

					GetGameInstance<UNeutronGameInstance>()->SetGameOnline(ENovaConstants::DefaultLevel, Online);
				}));
	}
}

void ANovaPlayerController::ClientLoadPlayer()
{
	NLOG("ANovaPlayerController::ClientLoadPlayer");

	// Check for save data
	UNeutronSaveManager* SaveManager = UNeutronSaveManager::Get();
	NCHECK(SaveManager);

#if WITH_EDITOR

	// Ensure valid save data exists even if the game was loaded directly on a map (PIE client)
	if (GetLocalRole() != ROLE_Authority && !SaveManager->HasLoadedSaveData())
	{
		LoadGame("PIE");
	}

#endif    // WITH_EDITOR

	NCHECK(UNeutronSaveManager::Get()->HasLoadedSaveData());

	// Serialize the save data and spawn the player actors on the server
	TSharedPtr<FJsonObject>   JsonData;
	TSharedPtr<FNovaGameSave> GameSaveData = SaveManager->GetCurrentSaveData<FNovaGameSave>();
	ServerLoadPlayer(GameSaveData->PlayerData);
}

void ANovaPlayerController::ServerLoadPlayer_Implementation(const FNovaPlayerSave& PlayerSaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::ServerLoadPlayer_Implementation");

	// Load
	Load(PlayerSaveData);
}

void ANovaPlayerController::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft)
{
	NLOG("ANovaPlayerController::UpdateSpacecraft ('%s')", *GetRoleString(this));

	// Update the player spacecraft
	if (GetLocalRole() == ROLE_Authority)
	{
		ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		NCHECK(IsValid(GameState));

		GameState->UpdatePlayerSpacecraft(Spacecraft, true);

		ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
		NCHECK(SpacecraftPawn);

		SpacecraftPawn->SetSpacecraftIdentifier(Spacecraft.Identifier);
	}

	// Tell the server
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerUpdateSpacecraft(Spacecraft);
	}
}

void ANovaPlayerController::ServerUpdateSpacecraft_Implementation(const FNovaSpacecraft& Spacecraft)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::ServerUpdateSpacecraft_Implementation");

	UpdateSpacecraft(Spacecraft);
}

void ANovaPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANovaPlayerController, Credits);
}

/*----------------------------------------------------
    Menus
----------------------------------------------------*/

void ANovaPlayerController::EnterPhotoMode(FName ActionName)
{
	NLOG("ANovaPlayerController::EnterPhotoMode");

	UNeutronMenuManager::Get()->CloseMenu(FNeutronAsyncAction::CreateLambda(
		[=]()
		{
			PhotoModeAction = ActionName;
			SetCameraState(ENovaPlayerCameraState::PhotoMode);
		}));
}

void ANovaPlayerController::ExitPhotoMode()
{
	NLOG("ANovaPlayerController::ExitPhotoMode");

	UNeutronMenuManager::Get()->OpenMenu(FNeutronAsyncAction::CreateLambda(
		[=]()
		{
			PhotoModeAction = NAME_None;
			SetCameraState(ENovaPlayerCameraState::Default);
		}));
}

/*----------------------------------------------------
    Getters
----------------------------------------------------*/

const FNovaSpacecraft* ANovaPlayerController::GetSpacecraft() const
{
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	ANovaGameState*      GameState      = GetWorld()->GetGameState<ANovaGameState>();

	if (IsValid(SpacecraftPawn) && IsValid(GameState))
	{
		FGuid PlayerSpacecraftIdentifier = SpacecraftPawn->GetSpacecraftIdentifier();
		return GameState->GetSpacecraft(PlayerSpacecraftIdentifier);
	}
	else
	{
		return nullptr;
	}
}

class ANovaSpacecraftPawn* ANovaPlayerController::GetSpacecraftPawn() const
{
	return GetPawn<class ANovaSpacecraftPawn>();
}

#undef LOCTEXT_NAMESPACE
