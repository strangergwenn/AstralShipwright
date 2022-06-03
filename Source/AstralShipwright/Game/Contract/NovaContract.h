// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "System/NovaContractManager.h"

/*----------------------------------------------------
    Contracts implementations
----------------------------------------------------*/

/** Test class for contracts */
class FNovaTutorialContract : public FNovaContract
{
public:

	FNovaTutorialContract();

	virtual void Initialize(class UNovaGameInstance* CurrentGameInstance) override;

	virtual TSharedRef<FJsonObject> Save() const override;

	virtual void Load(const TSharedPtr<FJsonObject>& Data) override;

	virtual void OnEvent(const FNovaContractEvent& Event) override;
};
