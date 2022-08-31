// Astral Shipwright - Gwennaël Arbona

#include "NovaPlayerController.h"
#include "NovaPlayerState.h"

#include "Game/NovaAsteroid.h"
#include "Game/NovaGameMode.h"
#include "Game/NovaGameState.h"
#include "Game/NovaGameUserSettings.h"
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

ANovaPlayerController::ANovaPlayerController() : Super(), PhotoModeAction(NAME_None)
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
	SaveData.Credits            = Credits;
	SaveData.UnlockedComponents = UnlockedComponents;

	return SaveData;
}

void ANovaPlayerController::Load(const FNovaPlayerSave& SaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::Load");

	// Store the save data so that the spacecraft pawn can fetch it later when it spawns
	FNovaSpacecraft NewSpacecraft = SaveData.Spacecraft;
	NewSpacecraft.UpdatePropulsionMetrics();
	NewSpacecraft.UpdateModuleGroups();
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

	// Load parts
	UnlockedComponents = SaveData.UnlockedComponents;
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
	// UNeutronContractManager::Get()->Load(SaveData->ContractManagerData);
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
				[=](UPostProcessComponent* Volume, TArray<class UMaterialInstanceDynamic*> Materials,
					const TSharedPtr<FNeutronPostProcessSettingBase>& Current, const TSharedPtr<FNeutronPostProcessSettingBase>& Target,
					float Alpha)
				{
					UNovaGameUserSettings*            GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
					const FNeutronPostProcessSetting* MyCurrent        = static_cast<const FNeutronPostProcessSetting*>(Current.Get());
					const FNeutronPostProcessSetting* MyTarget         = static_cast<const FNeutronPostProcessSetting*>(Target.Get());

					// Custom settings
					ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
					Materials[0]->SetScalarParameterValue("NoiseEnabled", GameUserSettings->EnableCameraDegradation ? 1.0f : 0.0f);
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
		// UNeutronContractManager::Get()->BeginPlay(this,    //
		//	FNeutronContractCreationCallback::CreateWeakLambda(this,
		//		[=](ENeutronContractType Type, UNeutronGameInstance* CurrentGameInstance)
		//		{
		//			// Create the contract
		//			TSharedPtr<FNeutronContract> Contract;
		//			if (Type == ENeutronContractType::Tutorial)
		//			{
		//				Contract = MakeShared<FNovaTutorialContract>();
		//			}
		//			else
		//			{
		//				NCHECK(false);
		//			}

		//			// Create the contract
		//			Contract->Initialize(CurrentGameInstance);

		//			return Contract;
		//		}));

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
			SpacecraftPawn->SetSelectedCompartmentIndex(INDEX_NONE);
			SpacecraftPawn->SetHoveredCompartmentIndex(INDEX_NONE);
		}
	}
}

void ANovaPlayerController::GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const
{
	Super::GetPlayerViewPoint(Location, Rotation);

	ENovaPlayerCameraState CameraState = GetCameraState<ENovaPlayerCameraState>();

	// During cutscenes, use the closest camera viewpoint and focus the player ship
	if (IsReady() &&
		(CameraState == ENovaPlayerCameraState::CinematicSpacecraft || CameraState == ENovaPlayerCameraState::CinematicEnvironment ||
			CameraState == ENovaPlayerCameraState::CinematicBrake))
	{
		// Get points of interest nearby
		TArray<AActor*> Viewpoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlayerViewpoint::StaticClass(), Viewpoints);
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

		// Get viewpoint, asteroid
		FVector              PlayerLocation = GetPawn()->GetActorLocation();
		const ANovaAsteroid* Asteroid       = UNeutronActorTools::GetClosestActor<ANovaAsteroid>(this, PlayerLocation);

		if (PlayerViewpoint)
		{
			Location = PlayerViewpoint->GetActorLocation();

			// Spacecraft focus
			if (CameraState == ENovaPlayerCameraState::CinematicSpacecraft)
			{
				Rotation = (PlayerLocation - Location).Rotation();
			}

			// Environment panning shot
			else if (CameraState == ENovaPlayerCameraState::CinematicEnvironment)
			{
				float AnimationAlpha = FMath::Clamp(GetCurrentTimeInCameraState() / AnimationDuration, 0.0f, 1.0f);
				AnimationAlpha       = FMath::InterpEaseOut(-0.5f, 0.5f, AnimationAlpha, ENeutronUIConstants::EaseStandard);

				Rotation = PlayerViewpoint->GetActorRotation();
				Rotation +=
					FRotator(0, AnimationAlpha * PlayerViewpoint->CameraPanAmount, AnimationAlpha * PlayerViewpoint->CameraTiltAmount);
				Location += Rotation.Vector() * AnimationAlpha * PlayerViewpoint->CameraTravelingAmount;
			}
		}

		// Braking spacecraft entry
		if (CameraState == ENovaPlayerCameraState::CinematicBrake &&
			((StationDocks.Num() > 0 && IsValid(PlayerStart)) || IsValid(Asteroid)))
		{
			NCHECK(Planetariums.Num() > 0);

			// Define scene parameters
			double        ViewDistance   = StationDocks.Num() > 0 ? 10000.0 : 25000.0;
			const FVector TargetLocation = StationDocks.Num() > 0 ? PlayerStart->GetWaitingPointLocation() : Asteroid->GetActorLocation();
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

void ANovaPlayerController::OnCameraStateChanged()
{
	if (GetCameraState<ENovaPlayerCameraState>() == ENovaPlayerCameraState::FastForward)
	{
		UNeutronMenuManager::Get()->GetOverlay<SNovaOverlay>()->StartFastForward();
	}
	else
	{
		UNeutronMenuManager::Get()->GetOverlay<SNovaOverlay>()->StopFastForward();
	}
}

bool ANovaPlayerController::GetSharedTransitionMenuState(uint8 NewCameraState)
{
	switch ((ENovaPlayerCameraState) NewCameraState)
	{
		// UI enabled states
		default:
		case ENovaPlayerCameraState::Default:
			return true;
			break;

		// UI disabled states
		case ENovaPlayerCameraState::CinematicSpacecraft:
		case ENovaPlayerCameraState::CinematicEnvironment:
		case ENovaPlayerCameraState::CinematicBrake:
		case ENovaPlayerCameraState::FastForward:
			return false;
			break;
	}
}

void ANovaPlayerController::AnyKey(FKey Key)
{
	Super::AnyKey(Key);

	// Exit photo mode
	if (IsInPhotoMode())
	{
		if (UNeutronMenuManager::Get()->GetMenu()->IsActionKey(PhotoModeAction, Key) ||
			UNeutronMenuManager::Get()->GetMenu()->IsActionKey(FNeutronPlayerInput::MenuCancel, Key))
		{
			ExitPhotoMode();
		}
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
		SpacecraftPawn->RevertModifications();
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
    Progression
----------------------------------------------------*/

int32 ANovaPlayerController::GetComponentUnlockLevel() const
{
	return (UnlockedComponents.Num() / 2) + 1;
}

bool ANovaPlayerController::IsComponentUnlockable(const UNovaTradableAssetDescription* Asset, FText* Help) const
{
	if (!GetSpacecraftPawn()->IsDocked())
	{
		if (Help)
		{
			*Help = LOCTEXT("CantUnlockNotDocked", "Components can only be unlocked while docked");
		}
		return false;
	}
	else if (IsComponentUnlocked(Asset))
	{
		if (Help)
		{
			*Help = LOCTEXT("CantUnlockAgain", "This component is already unlocked");
		}
		return false;
	}
	else if (!CanAffordTransaction(GetComponentUnlockCost(Asset)))
	{
		if (Help)
		{
			*Help = LOCTEXT("CantUnlockExpensive", "This component is too expensive to unlock");
		}
		return false;
	}
	else if (Asset->UnlockLevel > GetComponentUnlockLevel())
	{
		if (Help)
		{
			*Help = LOCTEXT("CantUnlockUnavailable", "This component isn't available to unlock yet");
		}
		return false;
	}
	else
	{
		return true;
	}
}

bool ANovaPlayerController::IsComponentUnlocked(const UNovaTradableAssetDescription* Asset) const
{
	return Asset->UnlockLevel == 0 || UnlockedComponents.Contains(Asset->Identifier);
}

FNovaCredits ANovaPlayerController::GetComponentUnlockCost(int32 Level) const
{
	return 2500 * Level;
}

FNovaCredits ANovaPlayerController::GetComponentUnlockCost(const UNovaTradableAssetDescription* Asset) const
{
	return GetComponentUnlockCost(Asset->UnlockLevel);
}

void ANovaPlayerController::UnlockComponent(const UNovaTradableAssetDescription* Asset)
{
	ProcessTransaction(GetComponentUnlockCost(Asset));
	UnlockedComponents.Add(Asset->Identifier);
	SaveGame();
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
