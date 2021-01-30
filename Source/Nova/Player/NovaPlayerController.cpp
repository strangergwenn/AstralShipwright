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

#include "Nova/Spacecraft/NovaSpacecraftMovementComponent.h"

#include "Nova/Tools/NovaActorTools.h"
#include "Nova/UI/Menu/NovaOverlay.h"
#include "Nova/Nova.h"

#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Components/PostProcessComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SkyLightComponent.h"

#include "Engine/LocalPlayer.h"
#include "Engine/SpotLight.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"


#define LOCTEXT_NAMESPACE "ANovaPlayerController"


/*----------------------------------------------------
	Constructors
----------------------------------------------------*/

ANovaPlayerViewpoint::ANovaPlayerViewpoint()
	: Super()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
}

ANovaPlayerController::ANovaPlayerController()
	: Super()
	, LastNetworkError(ENovaNetworkError::Success)
	, IsInCutscene(false)
	, IsLoadingStreamingLevel(false)
	, CurrentStreamingLevelIndex(0)
{
	// Create the post-processing manager
	PostProcessComponent = CreateDefaultSubobject<UNovaPostProcessComponent>(TEXT("PostProcessComponent"));

	// Default settings
	TSharedPtr<FNovaPostProcessSetting> DefaultSettings = MakeShareable(new FNovaPostProcessSetting);
	PostProcessComponent->RegisterPreset(ENovaPostProcessPreset::Neutral, DefaultSettings);

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
	ANovaSpacecraftPawn* SpacecraftPawn = GetSpacecraftPawn();
	NCHECK(SpacecraftPawn);
	SaveData->Spacecraft = SpacecraftPawn->GetSpacecraft();

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

	// Store the save data so that the spacecraft pawn can fetch it later when it spawns
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
			ANovaSpacecraftPawn::SerializeJson(SaveData->Spacecraft, FactoryJsonData, ENovaSerialize::DataToJson);
		}
		JsonData->SetObjectField("Spacecraft", FactoryJsonData);
	}
	else
	{
		SaveData = MakeShareable(new FNovaPlayerSave);

		TSharedPtr<FJsonObject> FactoryJsonData = JsonData->GetObjectField("Spacecraft");
		ANovaSpacecraftPawn::SerializeJson(SaveData->Spacecraft, FactoryJsonData, ENovaSerialize::JsonToData);
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

			// TODO : this should be dependent on save data
			LoadStreamingLevel("Station");
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
	if (IsInCutscene)
	{
		TArray<AActor*> Viewpoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANovaPlayerViewpoint::StaticClass(), Viewpoints);

		FVector ViewpointLocation = FVector::ZeroVector;
		if (Viewpoints.Num())
		{
			UNovaActorTools::SortActorsByClosestDistance(Viewpoints, GetPawn()->GetActorLocation());
			ViewpointLocation = Viewpoints[0]->GetActorLocation();
		}

		Location = ViewpointLocation;
		Rotation = (GetPawn()->GetActorLocation() - ViewpointLocation).Rotation();
	}
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

	auto EndCutscene = [=]()
	{
		GetMenuManager()->OpenMenu(FNovaAsyncAction::CreateLambda([=]()
			{
				GetSpacecraftPawn()->ResetView();
				IsInCutscene = false;
			})
		);
	};

	GetMenuManager()->CloseMenu(FNovaAsyncAction::CreateLambda([=]()
		{
			IsInCutscene = true;
			LoadStreamingLevel("Station");
		}),
		FNovaAsyncCondition::CreateLambda([=]()
		{
			if (!IsLoadingStreamingLevel)
			{
				UNovaSpacecraftMovementComponent* MovementComponent = Cast<UNovaSpacecraftMovementComponent>(
					GetSpacecraftPawn()->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
				NCHECK(MovementComponent);

				// TODO move elsewhere
				for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
				{
					MovementComponent->Dock(*It, FSimpleDelegate::CreateLambda(EndCutscene));
					break;
				}

				return true;
			}
			else
			{
				return false;
			}
		}));
}

void ANovaPlayerController::Undock()
{
	NLOG("ANovaPlayerController::Undock");

	auto EndCutscene = [=]()
	{
		GetMenuManager()->OpenMenu(FNovaAsyncAction::CreateLambda([=]()
			{
				IsInCutscene = false;
				UnloadStreamingLevel("Station");
				GetSpacecraftPawn()->ResetView();
			}),
			FNovaAsyncCondition::CreateLambda([=]()
			{
				return !IsLoadingStreamingLevel;
			})
		);
	};

	auto StartCutscene = [=]()
	{
		IsInCutscene = true;

		UNovaSpacecraftMovementComponent* MovementComponent = Cast<UNovaSpacecraftMovementComponent>(
			GetSpacecraftPawn()->GetComponentByClass(UNovaSpacecraftMovementComponent::StaticClass()));
		NCHECK(MovementComponent);

		MovementComponent->Undock(FSimpleDelegate::CreateLambda(EndCutscene));
	};

	GetMenuManager()->CloseMenu(FNovaAsyncAction::CreateLambda(StartCutscene));
}


/*----------------------------------------------------
	Level loading
----------------------------------------------------*/

bool ANovaPlayerController::LoadStreamingLevel(FName SectorLevel)
{
	if (SectorLevel != NAME_None)
	{
		NLOG("ANovaPlayerController::LoadStreamingLevel : Loading streaming level '%s'", *SectorLevel.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget = this;
		Info.ExecutionFunction = "OnLevelLoaded";
		Info.UUID = CurrentStreamingLevelIndex;
		Info.Linkage = 0;

		UGameplayStatics::LoadStreamLevel(this, SectorLevel, true, false, Info);
		CurrentStreamingLevelIndex++;
		IsLoadingStreamingLevel = true;
		return false;
	}
	return true;
}

void ANovaPlayerController::UnloadStreamingLevel(FName SectorLevel)
{
	if (SectorLevel != NAME_None)
	{
		NLOG("ANovaPlayerController::UnloadStreamingLevel : Unloading streaming level '%s'", *SectorLevel.ToString());

		FLatentActionInfo Info;
		Info.CallbackTarget = this;
		Info.ExecutionFunction = "OnLevelUnloaded";
		Info.UUID = CurrentStreamingLevelIndex;
		Info.Linkage = 0;

		UGameplayStatics::UnloadStreamLevel(this, SectorLevel, Info, false);
		CurrentStreamingLevelIndex++;
		IsLoadingStreamingLevel = true;
	}
}

void ANovaPlayerController::OnLevelLoaded()
{
	NLOG("ANovaPlayerController::OnLevelLoaded");

	IsLoadingStreamingLevel = false;
}

void ANovaPlayerController::OnLevelUnLoaded()
{
	NLOG("ANovaPlayerController::OnLevelUnLoaded");

	IsLoadingStreamingLevel = false;
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
		FNovaAsyncAction::CreateLambda([=]()
		{
			GetGameInstance<UNovaGameInstance>()->StartGame(SaveName, Online);
		})
	);
}

void ANovaPlayerController::SetGameOnline(bool Online)
{
	NLOG("ANovaPlayerController::SetGameOnline : online = %d", Online);

	GetMenuManager()->RunAction(ENovaLoadingScreen::Launch,
		FNovaAsyncAction::CreateLambda([=]()
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
			FNovaAsyncAction::CreateLambda([=]()
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
			FNovaAsyncAction::CreateLambda([=]()
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
		FNovaAsyncAction::CreateLambda([=]()
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
		FNovaAsyncAction::CreateLambda([=]()
			{
				GetGameInstance<UNovaGameInstance>()->JoinSearchResult(InviteResult);
			})
	);
}

bool ANovaPlayerController::IsReady() const
{
	return !IsLoadingStreamingLevel
		&& (IsOnMainMenu() || (IsValid(GetSpacecraftPawn()) && GetSpacecraftPawn()->GetSpacecraft().IsValid()));
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
				FNovaAsyncAction::CreateLambda([=]()
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
