// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"

#include "NovaGameTypes.h"
#include "NovaGameState.h"

#include "Player/NovaPlayerController.h"
#include "System/NovaContractManager.h"
#include "System/NovaSaveManager.h"

#include "NovaSaveData.generated.h"

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

/** Save data */
USTRUCT()
struct FNovaGameSave : public FNovaSaveDataBase
{
	GENERATED_BODY()

	UPROPERTY()
	FNovaPlayerSave PlayerData;

	UPROPERTY()
	FNovaGameStateSave GameStateData;

	UPROPERTY()
	FNovaContractManagerSave ContractManagerData;
};
