// Astral Shipwright - Gwennaël Arbona

#include "NovaGameInstance.h"

#include "NovaAssetManager.h"
#include "NovaContractManager.h"
#include "NovaMenuManager.h"
#include "NovaPostProcessManager.h"
#include "NovaSoundManager.h"
#include "NovaSaveManager.h"
#include "NovaSessionsManager.h"

#include "Game/NovaGameState.h"
#include "Game/Settings/NovaGameUserSettings.h"
#include "Game/Settings/NovaWorldSettings.h"
#include "Player/NovaGameViewportClient.h"

#include "UI/NovaUI.h"
#include "Nova.h"

#include "Kismet/GameplayStatics.h"
#include "GameMapsSettings.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "UNovaGameInstance"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaGameInstance::UNovaGameInstance() : Super()
{}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void UNovaGameInstance::Init()
{
	Super::Init();

	// Apply user settings
	UNovaGameUserSettings* GameUserSettings = Cast<UNovaGameUserSettings>(GEngine->GetGameUserSettings());
	GameUserSettings->ApplyCustomGraphicsSettings();

	// Setup connection screen
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UNovaGameInstance::PreLoadMap);

	// Create asset manager
	AssetManager = NewObject<UNovaAssetManager>(this, UNovaAssetManager::StaticClass(), TEXT("AssetManager"));
	NCHECK(AssetManager);
	AssetManager->Initialize(this);

	// Create the contract manager
	ContractManager = NewObject<UNovaContractManager>(this, UNovaContractManager::StaticClass(), TEXT("ContractManager"));
	NCHECK(ContractManager);
	ContractManager->Initialize(this);

	// Create the menu manager
	MenuManager = NewObject<UNovaMenuManager>(this, UNovaMenuManager::StaticClass(), TEXT("MenuManager"));
	NCHECK(MenuManager);
	MenuManager->Initialize(this);

	// Create the post-process manager
	PostProcessManager = NewObject<UNovaPostProcessManager>(this, UNovaPostProcessManager::StaticClass(), TEXT("PostProcessManager"));
	NCHECK(PostProcessManager);
	PostProcessManager->Initialize(this);

	// Create save manager
	SaveManager = NewObject<UNovaSaveManager>(this, UNovaSaveManager::StaticClass(), TEXT("SaveManager"));
	NCHECK(SaveManager);
	SaveManager->Initialize(this);

	// Create the sessions  manager
	SessionsManager = NewObject<UNovaSessionsManager>(this, UNovaSessionsManager::StaticClass(), TEXT("SessionsManager"));
	NCHECK(SessionsManager);
	SessionsManager->Initialize(this);

	// Create the sound manager
	SoundManager = NewObject<UNovaSoundManager>(this, UNovaSoundManager::StaticClass(), TEXT("SoundManager"));
	NCHECK(SoundManager);
	SoundManager->Initialize(this);
}

void UNovaGameInstance::Shutdown()
{
	SessionsManager->Finalize();
	Super::Shutdown();
}

void UNovaGameInstance::PreLoadMap(const FString& InMapName)
{
	UNovaGameViewportClient* Viewport = Cast<UNovaGameViewportClient>(GetWorld()->GetGameViewport());
	if (Viewport)
	{
		Viewport->ShowLoadingScreen();
	}
}

/*----------------------------------------------------
    Game flow
----------------------------------------------------*/

void UNovaGameInstance::SetGameOnline(FString URL, bool Online)
{
	NLOG("UNovaGameInstance::SetGameOnline : '%s', online = %d", *URL, Online);

	SessionsManager->ClearErrors();

	// We want to be online and presumably aren't, start a new online session and then travel to the level
	if (Online)
	{
		SessionsManager->StartSession(URL, ENovaConstants::MaxPlayerCount, true);
	}

	// We are on the main menu and are staying offline, simply travel there
	else if (Cast<ANovaWorldSettings>(GetWorld()->GetWorldSettings())->IsMainMenuMap())
	{
		GetFirstLocalPlayerController()->ClientTravel(URL, ETravelType::TRAVEL_Absolute, false);
	}

	// We want to be offline and presumably aren't, kill the session and then travel to the level
	else
	{
		SessionsManager->EndSession(URL);
	}
}

void UNovaGameInstance::GoToMainMenu()
{
	NLOG("UNovaGameInstance::GoToMainMenu");

	UNovaSaveManager::Get()->ReleaseCurrentSaveData();

	UGameplayStatics::OpenLevel(GetWorld(), FName(*UGameMapsSettings::GetGameDefaultMap()), true);
}

void UNovaGameInstance::ServerTravel(FString URL)
{
	GetWorld()->ServerTravel(URL + TEXT("?listen"), true);
}

#undef LOCTEXT_NAMESPACE
