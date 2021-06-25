// Nova project - Gwennaël Arbona

#include "NovaGameInstance.h"

#include "NovaAssetManager.h"
#include "NovaContractManager.h"
#include "NovaMenuManager.h"
#include "NovaSaveManager.h"
#include "NovaSessionsManager.h"

#include "Nova/Game/NovaGameState.h"
#include "Nova/Game/NovaGameUserSettings.h"
#include "Nova/Player/NovaPlayerController.h"
#include "Nova/Player/NovaGameViewportClient.h"

#include "Nova/UI/NovaUI.h"
#include "Nova/Nova.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "UNovaGameInstance"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

UNovaGameInstance::UNovaGameInstance() : Super(), CurrentSaveData(nullptr), TimeOfLastSave(0)
{}

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

struct FNovaGameSave
{
	TSharedPtr<struct FNovaPlayerSave>          PlayerData;
	TSharedPtr<struct FNovaGameStateSave>       GameStateData;
	TSharedPtr<struct FNovaContractManagerSave> ContractManagerData;
};

TSharedPtr<FNovaGameSave> UNovaGameInstance::Save(const ANovaPlayerController* PC)
{
	TSharedPtr<FNovaGameSave> Save = CurrentSaveData;

	// Save the player
	Save->PlayerData = PC->Save();

	// Save the world
	if (PC->GetLocalRole() == ROLE_Authority)
	{
		ANovaGameState* GameState = GetWorld()->GetGameState<ANovaGameState>();
		if (IsValid(GameState))
		{
			Save->GameStateData = GameState->Save();
		}
	}

	// Save contracts
	Save->ContractManagerData = ContractManager->Save();

	// Reset the save time
	TimeOfLastSave = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());

	return Save;
}

void UNovaGameInstance::Load(TSharedPtr<FNovaGameSave> SaveData)
{
	CurrentSaveData = SaveData;

	// Only load contracts right away, other classes will fetch their stuff when they need it
	ContractManager->Load(GetContractManagerSave());

	// Reset the save time
	TimeOfLastSave = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());
}

void UNovaGameInstance::SerializeJson(TSharedPtr<FNovaGameSave>& SaveData, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	// Writing to save
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		TSharedPtr<FJsonObject> PlayerJsonData = MakeShared<FJsonObject>();
		ANovaPlayerController::SerializeJson(SaveData->PlayerData, PlayerJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("Player", PlayerJsonData);

		TSharedPtr<FJsonObject> GameStateJsonData = MakeShared<FJsonObject>();
		ANovaGameState::SerializeJson(SaveData->GameStateData, GameStateJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("GameState", GameStateJsonData);

		TSharedPtr<FJsonObject> ContractManagerJsonData = MakeShared<FJsonObject>();
		UNovaContractManager::SerializeJson(SaveData->ContractManagerData, ContractManagerJsonData, ENovaSerialize::DataToJson);
		JsonData->SetObjectField("ContractManager", ContractManagerJsonData);
	}

	// Reading from save
	else
	{
		SaveData = MakeShared<FNovaGameSave>();

		TSharedPtr<FJsonObject> PlayerJsonData =
			JsonData->HasTypedField<EJson::Object>("Player") ? JsonData->GetObjectField("Player") : MakeShared<FJsonObject>();
		ANovaPlayerController::SerializeJson(SaveData->PlayerData, PlayerJsonData, ENovaSerialize::JsonToData);

		TSharedPtr<FJsonObject> GameStateJsonData =
			JsonData->HasTypedField<EJson::Object>("GameState") ? JsonData->GetObjectField("GameState") : MakeShared<FJsonObject>();
		ANovaGameState::SerializeJson(SaveData->GameStateData, GameStateJsonData, ENovaSerialize::JsonToData);

		TSharedPtr<FJsonObject> ContractManagerJsonData = JsonData->HasTypedField<EJson::Object>("ContractManager")
															? JsonData->GetObjectField("ContractManager")
															: MakeShared<FJsonObject>();
		UNovaContractManager::SerializeJson(SaveData->ContractManagerData, ContractManagerJsonData, ENovaSerialize::JsonToData);
	}
}

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
	AssetManager->Initialize();

	// Create save manager
	SaveManager = NewObject<UNovaSaveManager>(this, UNovaSaveManager::StaticClass(), TEXT("SaveManager"));
	NCHECK(SaveManager);

	// Create the sessions  manager
	SessionsManager = NewObject<UNovaSessionsManager>(this, UNovaSessionsManager::StaticClass(), TEXT("SessionsManager"));
	NCHECK(SessionsManager);
	SessionsManager->Initialize(this);

	// Create the menu manager
	MenuManager = NewObject<UNovaMenuManager>(this, UNovaMenuManager::StaticClass(), TEXT("MenuManager"));
	NCHECK(MenuManager);
	MenuManager->Initialize(this);

	// Create the contract manager
	ContractManager = NewObject<UNovaContractManager>(this, UNovaContractManager::StaticClass(), TEXT("ContractManager"));
	NCHECK(ContractManager);
	ContractManager->Initialize(this);
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
    Game save handling
----------------------------------------------------*/

void UNovaGameInstance::StartGame(FString SaveName, bool Online)
{
	NLOG("UNovaGameInstance::StartGame");

	LoadGame(SaveName);

	SetGameOnline(ENovaConstants::DefaultLevel, Online);
}

void UNovaGameInstance::LoadGame(FString SaveName)
{
	NLOG("UNovaGameInstance::LoadGame : loading from '%s'", *SaveName);

	CurrentSaveData     = nullptr;
	CurrentSaveFileName = SaveName;

	// Read and de-serialize all data, without actually loading objects
	CurrentSaveData = SaveManager->LoadGame(SaveName);
	NCHECK(CurrentSaveData);

	Load(CurrentSaveData);
}

void UNovaGameInstance::SaveGame(ANovaPlayerController* PC, bool Synchronous)
{
	NLOG("UNovaGameInstance::SaveGame : synchronous %d", Synchronous);

	NCHECK(PC);

	// Get an updated save
	CurrentSaveData = Save(PC);

	// Write to file
	SaveGameToFile(Synchronous);
	PC->Notify(LOCTEXT("SavedGame", "Game saved"), FText(), ENovaNotificationType::Save);
}

void UNovaGameInstance::SaveGameToFile(bool Synchronous)
{
	NLOG("UNovaGameInstance::SaveGameToFile : synchronous %d", Synchronous);

	if (CurrentSaveData.IsValid())
	{
		if (Synchronous)
		{
			SaveManager->SaveGame(CurrentSaveFileName, CurrentSaveData);
		}
		else
		{
			SaveManager->SaveGameAsync(CurrentSaveFileName, CurrentSaveData);
		}
	}
}

bool UNovaGameInstance::HasSave() const
{
	return CurrentSaveData.IsValid();
}

TSharedPtr<FNovaPlayerSave> UNovaGameInstance::GetPlayerSave()
{
	NCHECK(CurrentSaveData);
	return CurrentSaveData->PlayerData;
}

TSharedPtr<struct FNovaGameStateSave> UNovaGameInstance::GetWorldSave()
{
	NCHECK(CurrentSaveData);
	return CurrentSaveData->GameStateData;
}

TSharedPtr<FNovaContractManagerSave> UNovaGameInstance::GetContractManagerSave()
{
	NCHECK(CurrentSaveData);
	return CurrentSaveData->ContractManagerData;
}

/*----------------------------------------------------
    Game flow
----------------------------------------------------*/

void UNovaGameInstance::SetGameOnline(FString URL, bool Online)
{
	NLOG("UNovaGameInstance::SetGameOnline : '%s', online = %d", *URL, Online);

	SessionsManager->ClearErrors();
	ANovaPlayerController* PC = Cast<ANovaPlayerController>(GetFirstLocalPlayerController());

	// We want to be online and presumably aren't, start a new online session and then travel to the level
	if (Online)
	{
		SessionsManager->StartSession(URL, ENovaConstants::MaxPlayerCount, true);
	}

	// We are on the main menu and are staying offline, simply travel there
	else if (PC->IsOnMainMenu())
	{
		PC->ClientTravel(URL, ETravelType::TRAVEL_Absolute, false);
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

	CurrentSaveData = nullptr;

	UGameplayStatics::OpenLevel(GetWorld(), FName("MenuMap"), true);
}

void UNovaGameInstance::ServerTravel(FString URL)
{
	GetWorld()->ServerTravel(URL + TEXT("?listen"), true);
}

#undef LOCTEXT_NAMESPACE
