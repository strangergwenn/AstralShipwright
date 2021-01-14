// Nova project - Gwennaël Arbona

#include "NovaPlayerController.h"
#include "NovaMenuManager.h"
#include "NovaGameViewportClient.h"

#include "Nova/Actor/NovaTurntablePawn.h"

#include "Nova/Game/NovaAssetCatalog.h"
#include "Nova/Game/NovaContractManager.h"
#include "Nova/Game/NovaGameInstance.h"
#include "Nova/Game/NovaGameMode.h"
#include "Nova/Game/NovaGameUserSettings.h"
#include "Nova/Game/NovaSaveManager.h"
#include "Nova/Game/NovaWorldSettings.h"

#include "Nova/UI/Menu/NovaOverlay.h"
#include "Nova/Nova.h"

#include "GameFramework/PlayerState.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Misc/CommandLine.h"

#include "Engine/LocalPlayer.h"
#include "Engine/SpotLight.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"


#define LOCTEXT_NAMESPACE "ANovaPlayerController"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

ANovaPlayerController::ANovaPlayerController()
	: Super()
	, TravelState(ENovaTravelState::None)
	, LastNetworkError(ENovaNetworkError::Success)
	, IsOnDeathScreen(false)
{
	// Create the post-processing manager
	PostProcessComponent = CreateDefaultSubobject<UNovaPostProcessComponent>(TEXT("PostProcessComponent"));

	// Default settings
	TSharedPtr<FNovaPostProcessSetting> DefaultSettings = MakeShareable(new FNovaPostProcessSetting);
	PostProcessComponent->RegisterPreset(ENovaPostProcessPreset::Neutral, DefaultSettings);

	// Damage effect preset
	TSharedPtr<FNovaPostProcessSetting> DangerSettings = MakeShareable(new FNovaPostProcessSetting);
	DangerSettings->SceneColorTint = FLinearColor(1.0f, 0.3f, 0.2f);
	DangerSettings->TransitionDuration = 0.1f;
	PostProcessComponent->RegisterPreset(ENovaPostProcessPreset::Damage, DangerSettings);

	// Initialize post-processing
	PostProcessComponent->Initialize(

		// Preset control
		FNovaPostProcessControl::CreateLambda([=]()
			{
				ENovaPostProcessPreset TargetPreset = ENovaPostProcessPreset::Neutral;

				return static_cast<int32>(TargetPreset);
			}),

		// Preset tick
		FNovaPostProcessUpdate::CreateLambda([=](UPostProcessComponent* Volume, UMaterialInstanceDynamic* Material,
			const TSharedPtr<FNovaPostProcessSettingBase>& Current, const TSharedPtr<FNovaPostProcessSettingBase>& Target, float Alpha)
			{
				UNovaGameUserSettings* GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
				const FNovaPostProcessSetting* MyCurrent = static_cast<const FNovaPostProcessSetting*>(Current.Get());
				const FNovaPostProcessSetting* MyTarget = static_cast<const FNovaPostProcessSetting*>(Target.Get());

				// Config-driven settings
				Volume->Settings.bOverride_BloomMethod = true;
				Volume->Settings.bOverride_ScreenPercentage = true;
				Volume->Settings.bOverride_ReflectionsType = true;
				Volume->Settings.BloomMethod = GameUserSettings->EnableCinematicBloom ? BM_FFT : BM_SOG;
				Volume->Settings.ScreenPercentage = GameUserSettings->ScreenPercentage;
				Volume->Settings.ReflectionsType = GameUserSettings->EnableRaytracedReflections ? EReflectionsType::RayTracing : EReflectionsType::ScreenSpace;
				Volume->Settings.RayTracingAO = GameUserSettings->EnableRaytracedAO;

				// Custom settings
				//Material->SetScalarParameterValue("ChromaIntensity", FMath::Lerp(Current.ChromaIntensity, Target.ChromaIntensity, Alpha));

				// Built-in settings (overrides)
				Volume->Settings.bOverride_AutoExposureMinBrightness = true;
				Volume->Settings.bOverride_AutoExposureMaxBrightness = true;
				Volume->Settings.bOverride_GrainIntensity = true;
				Volume->Settings.bOverride_SceneColorTint = true;

				// Built in settings (values)
				Volume->Settings.AutoExposureMinBrightness = FMath::Lerp(MyCurrent->AutoExposureBrightness, MyTarget->AutoExposureBrightness, Alpha);
				Volume->Settings.AutoExposureMaxBrightness = FMath::Lerp(MyCurrent->AutoExposureBrightness, MyTarget->AutoExposureBrightness, Alpha);
				Volume->Settings.GrainIntensity = FMath::Lerp(MyCurrent->GrainIntensity, MyTarget->GrainIntensity, Alpha);
				Volume->Settings.SceneColorTint = FMath::Lerp(MyCurrent->SceneColorTint, MyTarget->SceneColorTint, Alpha);
			})
	);
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
	TSharedPtr<FNovaPlayerSave> SaveData = MakeShareable(new FNovaPlayerSave);

	// Save the spacecraft
	ANovaSpacecraftAssembly* SpacecraftAssembly = GetSpacecraftAssembly();
	NCHECK(SpacecraftAssembly);
	SaveData->Spacecraft = SpacecraftAssembly->GetSpacecraft();

	return SaveData;
}

void ANovaPlayerController::Load(TSharedPtr<FNovaPlayerSave> SaveData)
{
	NLOG("ANovaPlayerController::Load");

	NCHECK(GetLocalRole() == ROLE_Authority);

	// Check out what we need
	ANovaGameMode* Game = Cast<ANovaGameMode>(GetWorld()->GetAuthGameMode());
	NCHECK(Game);
	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
	NCHECK(GameInstance);

	// Store the save data so that the assembly can fetch it later when it spawns
	Spacecraft = SaveData->Spacecraft;
}

void ANovaPlayerController::SerializeJson(TSharedPtr<FNovaPlayerSave>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShareable(new FJsonObject);

		TSharedPtr<FJsonObject> FactoryJsonData;
		if (SaveData)
		{
			ANovaSpacecraftAssembly::SerializeJson(SaveData->Spacecraft, FactoryJsonData, ENovaSerialize::DataToJson);
		}
		JsonData->SetObjectField("Spacecraft", FactoryJsonData);
	}
	else
	{
		SaveData = MakeShareable(new FNovaPlayerSave);

		TSharedPtr<FJsonObject> FactoryJsonData = JsonData->GetObjectField("Spacecraft");
		ANovaSpacecraftAssembly::SerializeJson(SaveData->Spacecraft, FactoryJsonData, ENovaSerialize::JsonToData);
	}
}


/*----------------------------------------------------
	Inherited
----------------------------------------------------*/

void ANovaPlayerController::BeginPlay()
{
	NLOG("ANovaPlayerController::BeginPlay");

	Super::BeginPlay();

	// Load save data
	if (IsLocalPlayerController())
	{
		if (!IsOnMainMenu())
		{
			ClientLoadPlayer();
		}

		GetMenuManager()->BeginPlay(this);
	}

	TravelState = ENovaTravelState::None;

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

		// Process the smooth travel system
		if (TravelURL.Len())
		{
			bool CanTravel = true;
			for (const ANovaPlayerController* OtherPlayer : TActorRange<ANovaPlayerController>(GetWorld()))
			{
				NCHECK(!OtherPlayer->IsPendingKillPending());
				if (!OtherPlayer->IsWaitingTravel())
				{
					CanTravel = false;
					break;
				}
			}

			if (CanTravel)
			{
				NLOG("ANovaPlayerController::PlayerTick : starting travel");

				UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
				GameInstance->ServerTravel(TravelURL);
				TravelURL = FString();
			}
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

		// Go to the death screen
		if (/** ded */ false && !IsOnDeathScreen)
		{
			NLOG("ANovaPlayerController::PlayerTick : showing death screen");

			IsOnDeathScreen = true;
			GetMenuManager()->RunWaitAction(ENovaLoadingScreen::Black,
				FNovaFadeAction::CreateLambda([=]()
					{
						GetMenuManager()->GetOverlay()->ShowDeathScreen(ENovaDamageType::Generic,
							FSimpleDelegate::CreateUObject(this, &ANovaPlayerController::DeathScreenFinished, ENovaDamageType::Generic));
					})
			);
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


/*----------------------------------------------------
	Travel
----------------------------------------------------*/

void ANovaPlayerController::SmoothServerTravel(const FString& URL)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::SmoothServerTravel");

	TravelState = ENovaTravelState::None;

	for (ANovaPlayerController* OtherPlayer : TActorRange<ANovaPlayerController>(GetWorld()))
	{
		OtherPlayer->ClientPrepareTravel();
	}

	TravelURL = URL;
}

void ANovaPlayerController::ClientPrepareTravel_Implementation()
{
	NLOG("ANovaPlayerController::ClientPrepareTravel_Implementation");

	GetMenuManager()->RunWaitAction(ENovaLoadingScreen::Launch,
		FNovaFadeAction::CreateLambda([=]()
			{
				TravelState = ENovaTravelState::Waiting;
				GetMenuManager()->GetOverlay()->HideDeathScreen();
				ServerTravelPrepared();
				NLOG("ANovaPlayerController::ClientTravelInternal_Implementation : done, waiting for travel");
			}),
		FNovaFadeCondition::CreateLambda([=]()
			{
				return TravelState != ENovaTravelState::Waiting;
			})
		);
}

bool ANovaPlayerController::ServerTravelPrepared_Validate()
{
	return true;
}

void ANovaPlayerController::ServerTravelPrepared_Implementation()
{
	NLOG("ANovaPlayerController::ServerTravelPrepared_Implementation");

	TravelState = ENovaTravelState::Waiting;
}

void ANovaPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	NLOG("ANovaPlayerController::PreClientTravel");

	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	GetMenuManager()->FreezeLoadingScreen();

	TravelState = ENovaTravelState::Traveling;
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
		GameInstance->LoadGame("1");
	}

#endif

	// Serialize the save data and spawn the player actors on the server
	TSharedPtr<FJsonObject> JsonData;
	TSharedPtr<FNovaPlayerSave> PlayerSaveData = GameInstance->GetPlayerSave();
	SerializeJson(PlayerSaveData, JsonData, ENovaSerialize::DataToJson);
	ServerLoadPlayer(UNovaSaveManager::JsonToString(JsonData));
}

bool ANovaPlayerController::ServerLoadPlayer_Validate(const FString& SerializedSaveData)
{
	return true;
}

void ANovaPlayerController::ServerLoadPlayer_Implementation(const FString& SerializedSaveData)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANovaPlayerController::ServerLoadPlayer");

	// Deserialize save data
	TSharedPtr<FNovaPlayerSave> SaveData;
	TSharedPtr<FJsonObject> JsonData = UNovaSaveManager::StringToJson(SerializedSaveData);
	SerializeJson(SaveData, JsonData, ENovaSerialize::JsonToData);

	// Load
	Load(SaveData);
}


/*----------------------------------------------------
	Game flow
----------------------------------------------------*/

void ANovaPlayerController::StartGame(FString SaveName, bool Online)
{
	NLOG("ANovaPlayerController::StartGame : loading from '%s', online = %d", *SaveName, Online);
	
	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch,
		FNovaFadeAction::CreateLambda([=]()
		{
			GetGameInstance<UNovaGameInstance>()->StartGame(SaveName, Online);
		})
	);
}

void ANovaPlayerController::SetGameOnline(bool Online)
{
	NLOG("ANovaPlayerController::SetGameOnline : online = %d", Online);

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch,
		FNovaFadeAction::CreateLambda([=]()
		{
			GetGameInstance<UNovaGameInstance>()->SetGameOnline(GetWorld()->GetName(), Online);
		})
	);
}

void ANovaPlayerController::GoToMainMenu()
{
	if (GetMenuManager()->IsIdle())
	{
		NLOG("ANovaPlayerController::GoToMainMenu");

		GetMenuManager()->RunAction(ENovaLoadingScreen::Black,
			FNovaFadeAction::CreateLambda([=]()
				{
					GetGameInstance<UNovaGameInstance>()->SaveGame(true);
					GetGameInstance<UNovaGameInstance>()->GoToMainMenu();
				})
		);
	}
}

void ANovaPlayerController::ExitGame()
{
	if (GetMenuManager()->IsIdle())
	{
		NLOG("ANovaPlayerController::ExitGame");

		GetMenuManager()->RunAction(ENovaLoadingScreen::Black,
			FNovaFadeAction::CreateLambda([=]()
				{
					FGenericPlatformMisc::RequestExit(false);
				})
		);
	}
}

void ANovaPlayerController::InviteFriend(TSharedRef<FOnlineFriend> Friend)
{
	NLOG("ANovaPlayerController::InviteFriend");

	UNovaGameInstance* GameInstance = GetGameInstance<UNovaGameInstance>();
	NCHECK(GameInstance);

	Notify(FText::FormatNamed(LOCTEXT("InviteFriend", "Invited {friend}"),
		TEXT("friend"), FText::FromString(Friend->GetDisplayName())), ENovaNotificationType::Info);

	GameInstance->InviteFriend(Friend->GetUserId());
}

void ANovaPlayerController::JoinFriend(TSharedRef<FOnlineFriend> Friend)
{
	NLOG("ANovaPlayerController::JoinFriend");

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch,
		FNovaFadeAction::CreateLambda([=]()
			{
				Notify(FText::FormatNamed(LOCTEXT("JoiningFriend", "Joining {friend}"),
					TEXT("friend"), FText::FromString(Friend->GetDisplayName())), ENovaNotificationType::Info);
				GetGameInstance<UNovaGameInstance>()->JoinFriend(Friend->GetUserId());
			})
	);
}

void ANovaPlayerController::AcceptInvitation(const FOnlineSessionSearchResult& InviteResult)
{
	NLOG("ANovaPlayerController::AcceptInvitation");

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch,
		FNovaFadeAction::CreateLambda([=]()
			{
				GetGameInstance<UNovaGameInstance>()->JoinSearchResult(InviteResult);
			})
	);
}

bool ANovaPlayerController::IsReady() const
{
	return IsOnMainMenu() || (IsValid(GetSpacecraftAssembly()) && GetSpacecraftAssembly()->GetSpacecraft().IsValid());
}

void ANovaPlayerController::DeathScreenFinished(ENovaDamageType Type)
{
	NLOG("ANovaPlayerController::DeathScreenFinished");

	GetMenuManager()->RunWaitAction(ENovaLoadingScreen::Black,
		FNovaFadeAction::CreateLambda([=]()
			{
				GetMenuManager()->GetOverlay()->HideDeathScreen();

				 // TODO : process "respawn", however that works
			}),
		FNovaFadeCondition::CreateLambda([=]()
			{
				if (IsReady())
				{
					IsOnDeathScreen = false;
					return true;
				}
				else
				{
					return false;
				}
			})
		);
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

void ANovaPlayerController::OnOpenMenu()
{
}

void ANovaPlayerController::OnCloseMenu()
{
}

void ANovaPlayerController::Notify(FText Text, ENovaNotificationType Type)
{
	GetMenuManager()->GetOverlay()->Notify(Text, Type);
}


/*----------------------------------------------------
	Input
----------------------------------------------------*/

void ANovaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Core bindings
	FInputActionBinding& AnyKeyBinding = InputComponent->BindAction("AnyKey", EInputEvent::IE_Pressed, this, &ANovaPlayerController::AnyKey);
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
	if (!IsOnDeathScreen)
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
			GetMenuManager()->RunAction(ENovaLoadingScreen::Launch,
				FNovaFadeAction::CreateLambda([=]()
					{
						Notify(FText::FormatNamed(LOCTEXT("JoinFriend", "Joining {session}"),
							TEXT("session"), FText::FromString(*Result.Session.GetSessionIdStr())),
							ENovaNotificationType::Info);

						GetGameInstance<UNovaGameInstance>()->JoinSearchResult(Result);
					})
			);
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
{
}

#endif

#undef LOCTEXT_NAMESPACE
