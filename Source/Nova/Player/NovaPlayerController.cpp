// Nova project - Gwennaël Arbona

#include "NovaPlayerController.h"
#include "NovaMenuManager.h"
#include "NovaPlayerState.h"
#include "NovaGameViewportClient.h"

#include "Nova/Actor/NovaTurntablePawn.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaContractManager.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameUserSettings.h"
#include "Nova/Game/NovaSaveManager.h"
#include "Nova/Game/NovaWorldSettings.h"

#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Tools/NovaActorTools.h"
#include "Nova/UI/Menu/NovaOverlay.h"
#include "Nova/Nova.h"

#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"

#include "GameFramework/PlayerState.h"
#include "Components/PostProcessComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkyLightComponent.h"

#include "Engine/LocalPlayer.h"
#include "Engine/LevelStreaming.h"
#include "Engine/SpotLight.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "ANovaPlayerController"

/*----------------------------------------------------
    Constructors
----------------------------------------------------*/

ANovaPlayerViewpoint::ANovaPlayerViewpoint() : Super()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
}

ANovaPlayerController::ANovaPlayerController()
	: Super(), LastNetworkError(ENovaNetworkError::Success), CameraState(ENovaPlayerCameraState::Default), SharedTransitionActive(false)
{
	// Create the post-processing manager
	PostProcessComponent = CreateDefaultSubobject<UNovaPostProcessComponent>(TEXT("PostProcessComponent"));

	// Default settings
	TSharedPtr<FNovaPostProcessSetting> DefaultSettings = MakeShared<FNovaPostProcessSetting>();
	PostProcessComponent->RegisterPreset(ENovaPostProcessPreset::Neutral, DefaultSettings);

	// Initialize post-processing
	PostProcessComponent->Initialize(

		// Preset control
		FNovaPostProcessControl::CreateLambda(
			[=]()
			{
				ENovaPostProcessPreset TargetPreset = ENovaPostProcessPreset::Neutral;

				return static_cast<int32>(TargetPreset);
			}),

		// Preset tick
		FNovaPostProcessUpdate::CreateLambda(
			[=](UPostProcessComponent* Volume, UMaterialInstanceDynamic* Material, const TSharedPtr<FNovaPostProcessSettingBase>& Current,
				const TSharedPtr<FNovaPostProcessSettingBase>& Target, float Alpha)
			{
				UNovaGameUserSettings*         GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
				const FNovaPostProcessSetting* MyCurrent        = static_cast<const FNovaPostProcessSetting*>(Current.Get());
				const FNovaPostProcessSetting* MyTarget         = static_cast<const FNovaPostProcessSetting*>(Target.Get());

				// Config-driven settings
				Volume->Settings.bOverride_BloomMethod      = true;
				Volume->Settings.bOverride_ScreenPercentage = true;
				Volume->Settings.bOverride_ReflectionsType  = true;
				Volume->Settings.BloomMethod                = GameUserSettings->EnableCinematicBloom ? BM_FFT : BM_SOG;
				Volume->Settings.ScreenPercentage           = GameUserSettings->ScreenPercentage;
				Volume->Settings.ReflectionsType =
					GameUserSettings->EnableRaytracedReflections ? EReflectionsType::RayTracing : EReflectionsType::ScreenSpace;
				Volume->Settings.RayTracingAO = GameUserSettings->EnableRaytracedAO;

				// Custom settings
		        // Material->SetScalarParameterValue("ChromaIntensity", FMath::Lerp(Current.ChromaIntensity, Target.ChromaIntensity,
		        // Alpha));

				// Built-in settings (overrides)
				Volume->Settings.bOverride_AutoExposureMinBrightness = true;
				Volume->Settings.bOverride_AutoExposureMaxBrightness = true;
				Volume->Settings.bOverride_GrainIntensity            = true;
				Volume->Settings.bOverride_SceneColorTint            = true;

				// Built in settings (values)
				Volume->Settings.AutoExposureMinBrightness =
					FMath::Lerp(MyCurrent->AutoExposureBrightness, MyTarget->AutoExposureBrightness, Alpha);
				Volume->Settings.AutoExposureMaxBrightness =
					FMath::Lerp(MyCurrent->AutoExposureBrightness, MyTarget->AutoExposureBrightness, Alpha);
				Volume->Settings.GrainIntensity = FMath::Lerp(MyCurrent->GrainIntensity, MyTarget->GrainIntensity, Alpha);
				Volume->Settings.SceneColorTint = FMath::Lerp(MyCurrent->SceneColorTint, MyTarget->SceneColorTint, Alpha);
			}));

	// Defaults
	ChaseCamBaseDistance         = 5000;
	ChaseCamAccelerationDistance = 5000;
}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaPlayerSave
{
	TSharedPtr<struct FNovaSpacecraft> Spacecraft;
};

TSharedPtr<FNovaPlayerSave> ANovaPlayerController::Save() const
{
	TSharedPtr<FNovaPlayerSave> SaveData = MakeShared<FNovaPlayerSave>();

	// Save the spacecraft
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	NCHECK(SpacecraftPawn);
	const FNovaSpacecraft* Spacecraft = GetSpacecraft();
	if (Spacecraft)
	{
		SaveData->Spacecraft = Spacecraft->GetSharedCopy();
	}

	return SaveData;
}

void ANovaPlayerController::Load(TSharedPtr<FNovaPlayerSave> SaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::Load");

	// Store the save data so that the spacecraft pawn can fetch it later when it spawns
	UpdateSpacecraft(*SaveData->Spacecraft.Get());
}

void ANovaPlayerController::SerializeJson(
	TSharedPtr<FNovaPlayerSave>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		TSharedPtr<FJsonObject> SpacecraftJsonData;
		if (SaveData)
		{
			FNovaSpacecraft::SerializeJson(SaveData->Spacecraft, SpacecraftJsonData, ENovaSerialize::DataToJson);
		}
		JsonData->SetObjectField("Spacecraft", SpacecraftJsonData);
	}
	else
	{
		SaveData = MakeShared<FNovaPlayerSave>();

		TSharedPtr<FJsonObject> FactoryJsonData = JsonData->GetObjectField("Spacecraft");
		FNovaSpacecraft::SerializeJson(SaveData->Spacecraft, FactoryJsonData, ENovaSerialize::JsonToData);
	}
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
		// Load save data, process local game startup
		if (!IsOnMainMenu())
		{
			ClientLoadPlayer();
		}

		GetMenuManager()->BeginPlay(this);
	}

	// Initialize persistent objects
	GetGameInstance<UNovaGameInstance>()->SetAcceptedInvitationCallback(
		FOnFriendInviteAccepted::CreateUObject(this, &ANovaPlayerController::AcceptInvitation));

#if WITH_EDITOR

	// Start a host session if requested through command line
	if (FParse::Param(FCommandLine::Get(), TEXT("host")))
	{
		FCommandLine::Set(TEXT(""));

		GetGameInstance<UNovaGameInstance>()->StartSession(ENovaConstants::DefaultLevel, ENovaConstants::MaxPlayerCount, true);
	}

#endif
}

void ANovaPlayerController::PawnLeavingGame()
{
	NLOG("ANovaPlayerController::PawnLeavingGame");

	SetPawn(nullptr);
}

void ANovaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalPlayerController())
	{
		// Process the menu system
		UNovaMenuManager* MenuManager = GetMenuManager();
		if (IsValid(MenuManager))
		{
			MenuManager->Tick(DeltaTime);
		}

		// Process FOV
		UNovaGameUserSettings* GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
		NCHECK(PlayerCameraManager);
		float FOV = PlayerCameraManager->GetFOVAngle();
		if (FOV != GameUserSettings->FOV)
		{
			NLOG("ANovaPlayerController::PlayerTick : new FOV %d", static_cast<int>(GameUserSettings->FOV));
			PlayerCameraManager->SetFOV(GameUserSettings->FOV);
		}

		// Show network errors
		UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
		NCHECK(GameInstance);
		if (GameInstance->GetNetworkError() != LastNetworkError)
		{
			LastNetworkError = GameInstance->GetNetworkError();
			if (LastNetworkError != ENovaNetworkError::Success)
			{
				Notify(GameInstance->GetNetworkErrorString(), ENovaNotificationType::Error);
			}
		}

		// Update contracts
		UNovaContractManager::Get()->OnEvent(FNovaContractEvent(ENovaContratEventType::Tick));

		// Update lights
		for (ASpotLight* Light : TActorRange<ASpotLight>(GetWorld()))
		{
			Light->GetLightComponent()->SetCastRaytracedShadow(GameUserSettings->EnableRaytracedShadows);
		}
		for (ASkyLight* Light : TActorRange<ASkyLight>(GetWorld()))
		{
			Light->GetLightComponent()->SetCastRaytracedShadow(GameUserSettings->EnableRaytracedShadows);
		}
	}
}

void ANovaPlayerController::GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const
{
	// During cutscenes, use the closest camera viewpoint and focus the player ship
	if (IsReady() &&
		(CameraState == ENovaPlayerCameraState::CinematicSpacecraft || CameraState == ENovaPlayerCameraState::CinematicEnvironment))
	{
		TArray<AActor*> Viewpoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlayerViewpoint::StaticClass(), Viewpoints);

		// Get the first viewpoint actor and extract its transform
		FVector  ViewpointLocation = FVector::ZeroVector;
		FRotator ViewpointRotation = FRotator::ZeroRotator;
		if (Viewpoints.Num())
		{
			UNovaActorTools::SortActorsByClosestDistance(Viewpoints, GetPawn()->GetActorLocation());
			ViewpointLocation = Viewpoints[0]->GetActorLocation();
			ViewpointRotation = Viewpoints[0]->GetActorRotation();
		}

		// Apply the results
		Location = ViewpointLocation;
		if (CameraState == ENovaPlayerCameraState::CinematicSpacecraft)
		{
			Rotation = (GetPawn()->GetActorLocation() - ViewpointLocation).Rotation();
		}
		else
		{
			Rotation = ViewpointRotation;
		}
	}

	// Chase cam
	else if (IsReady() && CameraState == ENovaPlayerCameraState::Chase)
	{
		const ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
		NCHECK(SpacecraftPawn);

		const FVector Backwards             = -SpacecraftPawn->GetActorForwardVector();
		const FVector SpacecraftLocation    = SpacecraftPawn->GetActorLocation();
		const float   SpacecraftExtent      = SpacecraftPawn->GetTurntableBounds().Value.Size();
		const float   MainDriveAcceleration = SpacecraftPawn->GetSpacecraftMovement()->GetMainDriveAcceleration();

		FVector BoundsOffset       = SpacecraftExtent * Backwards;
		FVector BaseOffset         = ChaseCamBaseDistance * Backwards;
		FVector AccelerationOffset = MainDriveAcceleration * ChaseCamAccelerationDistance * Backwards;

		Location = SpacecraftLocation + BoundsOffset + BaseOffset + AccelerationOffset;
		Rotation = (GetPawn()->GetActorLocation() - Location).Rotation();
	}

	// Default camera
	else
	{
		Super::GetPlayerViewPoint(Location, Rotation);
	}
}

/*----------------------------------------------------
    Gameplay
----------------------------------------------------*/

void ANovaPlayerController::Dock()
{
	NLOG("ANovaPlayerController::Dock");

	FSimpleDelegate EndCutscene = FSimpleDelegate::CreateLambda(
		[=]()
		{
			GetMenuManager()->OpenMenu(FNovaAsyncAction::CreateLambda(
				[=]()
				{
					GetSpacecraftPawn()->ResetView();
				}));
		});

	FNovaAsyncAction StartCutscene = FNovaAsyncAction::CreateLambda(
		[=]()
		{
			GetSpacecraftPawn()->GetSpacecraftMovement()->Dock(EndCutscene);
		});

	GetMenuManager()->CloseMenu(StartCutscene);
}

void ANovaPlayerController::Undock()
{
	NLOG("ANovaPlayerController::Undock");

	FSimpleDelegate EndCutscene = FSimpleDelegate::CreateLambda(
		[=]()
		{
			GetMenuManager()->OpenMenu(FNovaAsyncAction::CreateLambda(
				[=]()
				{
					GetSpacecraftPawn()->ResetView();
				}));
		});

	FNovaAsyncAction StartCutscene = FNovaAsyncAction::CreateLambda(
		[=]()
		{
			GetSpacecraftPawn()->GetSpacecraftMovement()->Undock(EndCutscene);
		});

	GetMenuManager()->CloseMenu(StartCutscene);
}

void ANovaPlayerController::SharedTransition(
	ENovaPlayerCameraState NewCameraState, FNovaAsyncAction StartAction, FNovaAsyncCondition Condition, FNovaAsyncAction FinishAction)
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
	FNovaAsyncAction Action = FNovaAsyncAction::CreateLambda(
		[=]()
		{
			CameraState = NewCameraState;
			ServerSharedTransitionReady();
			NLOG("ANovaPlayerController::ClientStartSharedTransition_Implementation : done, waiting for server");
		});

	// Condition : on server, when all clients have reported as ready
	// On client, when the server has signaled to stop
	FNovaAsyncCondition Condition = FNovaAsyncCondition::CreateLambda(
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
		case ENovaPlayerCameraState::Chase:
			GetMenuManager()->OpenMenu(Action, Condition);
			break;

		// UI disabled states
		case ENovaPlayerCameraState::CinematicSpacecraft:
		case ENovaPlayerCameraState::CinematicEnvironment:
		case ENovaPlayerCameraState::FastForward:
			GetMenuManager()->CloseMenu(Action, Condition);
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

bool ANovaPlayerController::IsLevelStreamingComplete() const
{
	ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
	if (!IsValid(GameState))
	{
		return false;
	}

	for (const ULevelStreaming* Level : GetWorld()->GetStreamingLevels())
	{
		if (Level->IsStreamingStatePending())
		{
			return false;
		}
		else if (Level->IsLevelLoaded())
		{
			FString LoadedLevelName = Level->GetWorldAssetPackageFName().ToString();
			LoadedLevelName.EndsWith(GameState->GetCurrentLevelName().ToString());
		}
	}

	return true;
}

/*----------------------------------------------------
    Server-side save
----------------------------------------------------*/

void ANovaPlayerController::ClientLoadPlayer()
{
	NLOG("ANovaPlayerController::ClientLoadPlayer");
	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
	NCHECK(GameInstance);

#if WITH_EDITOR

	// Ensure valid save data exists even if the game was loaded directly
	if (!IsOnMainMenu() && !GameInstance->HasSave())
	{
		GameInstance->LoadGame(GetLocalRole() == ROLE_Authority ? "1" : "PIE");
	}

#endif

	// Serialize the save data and spawn the player actors on the server
	TSharedPtr<FJsonObject>     JsonData;
	TSharedPtr<FNovaPlayerSave> PlayerSaveData = GameInstance->GetPlayerSave();
	SerializeJson(PlayerSaveData, JsonData, ENovaSerialize::DataToJson);
	ServerLoadPlayer(UNovaSaveManager::JsonToString(JsonData));
}

void ANovaPlayerController::ServerLoadPlayer_Implementation(const FString& SerializedSaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::ServerLoadPlayer_Implementation");

	// Deserialize save data
	TSharedPtr<FNovaPlayerSave> SaveData;
	TSharedPtr<FJsonObject>     JsonData = UNovaSaveManager::StringToJson(SerializedSaveData);
	SerializeJson(SaveData, JsonData, ENovaSerialize::JsonToData);

	// Load
	Load(SaveData);
}

void ANovaPlayerController::UpdateSpacecraft(const FNovaSpacecraft& Spacecraft)
{
	NLOG("ANovaPlayerController::UpdateSpacecraft ('%s')", *GetRoleString(this));

	// Update the player spacecraft
	if (GetLocalRole() == ROLE_Authority)
	{
		ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		NCHECK(IsValid(GameState));

		GetPlayerState<ANovaPlayerState>()->SetSpacecraftIdentifier(Spacecraft.Identifier);
		GameState->UpdateSpacecraft(Spacecraft, true);
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

/*----------------------------------------------------
    Game flow
----------------------------------------------------*/

void ANovaPlayerController::StartGame(FString SaveName, bool Online)
{
	NLOG("ANovaPlayerController::StartGame : loading from '%s', online = %d", *SaveName, Online);

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch, FNovaAsyncAction::CreateLambda(
																[=]()
																{
																	GetGameInstance<UNovaGameInstance>()->StartGame(SaveName, Online);
																}));
}

void ANovaPlayerController::SetGameOnline(bool Online)
{
	NLOG("ANovaPlayerController::SetGameOnline : online = %d", Online);

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch, FNovaAsyncAction::CreateLambda(
																[=]()
																{
																	GetGameInstance<UNovaGameInstance>()->SetGameOnline(
																		GetWorld()->GetName(), Online);
																}));
}

void ANovaPlayerController::GoToMainMenu()
{
	if (GetMenuManager()->IsIdle())
	{
		NLOG("ANovaPlayerController::GoToMainMenu");

		GetMenuManager()->RunAction(ENovaLoadingScreen::Black, FNovaAsyncAction::CreateLambda(
																   [=]()
																   {
																	   GetGameInstance<UNovaGameInstance>()->SaveGame(this, true);
																	   GetGameInstance<UNovaGameInstance>()->GoToMainMenu();
																   }));
	}
}

void ANovaPlayerController::ExitGame()
{
	if (GetMenuManager()->IsIdle())
	{
		NLOG("ANovaPlayerController::ExitGame");

		GetMenuManager()->RunAction(ENovaLoadingScreen::Black, FNovaAsyncAction::CreateLambda(
																   [=]()
																   {
																	   FGenericPlatformMisc::RequestExit(false);
																   }));
	}
}

void ANovaPlayerController::InviteFriend(TSharedRef<FOnlineFriend> Friend)
{
	NLOG("ANovaPlayerController::InviteFriend");

	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
	NCHECK(GameInstance);

	Notify(FText::FormatNamed(LOCTEXT("InviteFriend", "Invited {friend}"), TEXT("friend"), FText::FromString(Friend->GetDisplayName())),
		ENovaNotificationType::Info);

	GameInstance->InviteFriend(Friend->GetUserId());
}

void ANovaPlayerController::JoinFriend(TSharedRef<FOnlineFriend> Friend)
{
	NLOG("ANovaPlayerController::JoinFriend");

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch, FNovaAsyncAction::CreateLambda(
																[=]()
																{
																	Notify(FText::FormatNamed(LOCTEXT("JoiningFriend", "Joining {friend}"),
																			   TEXT("friend"), FText::FromString(Friend->GetDisplayName())),
																		ENovaNotificationType::Info);
																	GetGameInstance<UNovaGameInstance>()->JoinFriend(Friend->GetUserId());
																}));
}

void ANovaPlayerController::AcceptInvitation(const FOnlineSessionSearchResult& InviteResult)
{
	NLOG("ANovaPlayerController::AcceptInvitation");

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch, FNovaAsyncAction::CreateLambda(
																[=]()
																{
																	GetGameInstance<UNovaGameInstance>()->JoinSearchResult(InviteResult);
																}));
}

bool ANovaPlayerController::IsReady() const
{
	if (IsOnMainMenu())
	{
		return true;
	}
	else
	{
		// Check spacecraft pawn
		ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
		bool IsSpacecraftReady = IsValid(SpacecraftPawn) && GetSpacecraft() && SpacecraftPawn->GetSpacecraftMovement()->IsInitialized();

		// Check game state
		ANovaGameState* GameState        = GetWorld()->GetGameState<ANovaGameState>();
		bool            IsGameStateReady = IsValid(GameState) && GameState->GetCurrentArea() != nullptr;

		return IsLevelStreamingComplete() && IsSpacecraftReady && IsGameStateReady;
	}
}

/*----------------------------------------------------
    Menus
----------------------------------------------------*/

bool ANovaPlayerController::IsOnMainMenu() const
{
	return Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMainMenuMap();
}

bool ANovaPlayerController::IsMenuOnly() const
{
	return Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMenuMap();
}

void ANovaPlayerController::Notify(const FText& Text, ENovaNotificationType Type)
{
	GetMenuManager()->GetOverlay()->Notify(Text, Type);
}

void ANovaPlayerController::ShowTitle(const FText& Text)
{
	GetMenuManager()->GetOverlay()->ShowTitle(Text);
}

/*----------------------------------------------------
    Input
----------------------------------------------------*/

void ANovaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Core bindings
	FInputActionBinding& AnyKeyBinding =
		InputComponent->BindAction("AnyKey", EInputEvent::IE_Pressed, this, &ANovaPlayerController::AnyKey);
	AnyKeyBinding.bConsumeInput = false;
	InputComponent->BindAction(FNovaPlayerInput::MenuToggle, EInputEvent::IE_Released, this, &ANovaPlayerController::ToggleMenuOrQuit);
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		InputComponent->BindKey(EKeys::Enter, EInputEvent::IE_Released, this, &ANovaPlayerController::ToggleMenuOrQuit);
	}

#if WITH_EDITOR

	// Test bindings
	InputComponent->BindAction("TestJoinSession", EInputEvent::IE_Released, this, &ANovaPlayerController::TestJoin);
	InputComponent->BindAction("TestActor", EInputEvent::IE_Released, this, &ANovaPlayerController::TestActor);

#endif
}

void ANovaPlayerController::ToggleMenuOrQuit()
{
	if (!Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMenuMap())
	{
		if (IsOnMainMenu())
		{
			ExitGame();
		}
		else
		{
			UNovaMenuManager* MenuManager = GetMenuManager();

			if (!MenuManager->IsMenuOpen())
			{
				MenuManager->OpenMenu();
			}
			else
			{
				MenuManager->CloseMenu();
			}
		}
	}
}

void ANovaPlayerController::AnyKey(FKey Key)
{
	GetMenuManager()->SetUsingGamepad(Key.IsGamepadKey());
}

/*----------------------------------------------------
    Getters
----------------------------------------------------*/

const FNovaSpacecraft* ANovaPlayerController::GetSpacecraft() const
{
	const ANovaGameState* GameState        = GetWorld()->GetGameState<ANovaGameState>();
	ANovaPlayerState*     OwnedPlayerState = GetPlayerState<ANovaPlayerState>();

	if (IsValid(GameState) && IsValid(OwnedPlayerState))
	{
		FGuid PlayerSpacecraftIdentifier = GetPlayerState<ANovaPlayerState>()->GetSpacecraftIdentifier();
		return GameState->GetSpacecraft(PlayerSpacecraftIdentifier);
	}
	else
	{
		return nullptr;
	}
}

/*----------------------------------------------------
    Test code
----------------------------------------------------*/

#if WITH_EDITOR

void ANovaPlayerController::OnJoinRandomFriend(TArray<TSharedRef<FOnlineFriend>> FriendList)
{
	for (auto Friend : FriendList)
	{
		if (Friend.Get().GetDisplayName() == "Deimos Games" || Friend.Get().GetDisplayName() == "Stranger")
		{
			JoinFriend(Friend);
		}
	}
}

void ANovaPlayerController::OnJoinRandomSession(TArray<FOnlineSessionSearchResult> SearchResults)
{
	for (auto Result : SearchResults)
	{
		if (Result.Session.OwningUserId != GetLocalPlayer()->GetPreferredUniqueNetId())
		{
			GetMenuManager()->RunAction(
				ENovaLoadingScreen::Launch, FNovaAsyncAction::CreateLambda(
												[=]()
												{
													Notify(FText::FormatNamed(LOCTEXT("JoinFriend", "Joining {session}"), TEXT("session"),
															   FText::FromString(*Result.Session.GetSessionIdStr())),
														ENovaNotificationType::Info);

													GetGameInstance<UNovaGameInstance>()->JoinSearchResult(Result);
												}));
		}
	}
}

void ANovaPlayerController::TestJoin()
{
	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();

	if (GameInstance->GetOnlineSubsystemName() != TEXT("Null"))
	{
		GameInstance->SearchFriends(FOnFriendSearchComplete::CreateUObject(this, &ANovaPlayerController::OnJoinRandomFriend));
	}
	else
	{
		GameInstance->SearchSessions(true, FOnSessionSearchComplete::CreateUObject(this, &ANovaPlayerController::OnJoinRandomSession));
	}
}

void ANovaPlayerController::TestActor()
{}

#endif

#undef LOCTEXT_NAMESPACE
