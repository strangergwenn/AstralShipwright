// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"

#include "NovaGameTypes.h"
#include "NovaGameState.h"
#include "Player/NovaPlayerController.h"

#include "Neutron/System/NeutronContractManager.h"
#include "Neutron/System/NeutronSaveManager.h"

#include "NovaSaveData.generated.h"

/*----------------------------------------------------
    Loading & saving
----------------------------------------------------*/

/** Save data */
USTRUCT()
struct FNovaGameSave : public FNeutronSaveDataBase
{
	GENERATED_BODY()

	UPROPERTY()
	FNovaPlayerSave PlayerData;

	UPROPERTY()
	FNovaGameStateSave GameStateData;

	UPROPERTY()
	FNeutronContractManagerSave ContractManagerData;
};
