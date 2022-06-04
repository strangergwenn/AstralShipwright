// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Game/NovaGameTypes.h"

#include "NovaGameInstance.generated.h"

/** Game instance class */
UCLASS(ClassGroup = (Nova))
class UNovaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UNovaGameInstance();

	/*----------------------------------------------------
	    Inherited & callbacks
	----------------------------------------------------*/

	virtual void Init() override;

	virtual void Shutdown() override;

	void PreLoadMap(const FString& InMapName);

	/*----------------------------------------------------
	    Game flow
	----------------------------------------------------*/

	/** Restart the game from the level in save data */
	void SetGameOnline(FString URL, bool Online = true);

	/** Exit the session and go to the main menu */
	void GoToMainMenu();

	/** Change level on the server */
	void ServerTravel(FString URL);

private:

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// Asset manager object
	UPROPERTY()
	class UNovaAssetManager* AssetManager;

	// Contract manager object
	UPROPERTY()
	class UNovaContractManager* ContractManager;

	// Menu manager object
	UPROPERTY()
	class UNovaMenuManager* MenuManager;

	// Post-processing manager object
	UPROPERTY()
	class UNovaPostProcessManager* PostProcessManager;

	// Sessions manager object
	UPROPERTY()
	class UNovaSessionsManager* SessionsManager;

	// Save manager object
	UPROPERTY()
	class UNovaSaveManager* SaveManager;

	// Sound manager object
	UPROPERTY()
	class UNovaSoundManager* SoundManager;
};
