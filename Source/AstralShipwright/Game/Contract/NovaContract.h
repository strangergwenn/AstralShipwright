// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Neutron/System/NeutronContractManager.h"

/*----------------------------------------------------
    Contracts implementations
----------------------------------------------------*/

/** Test class for contracts */
class FNovaTutorialContract : public FNeutronContract
{
public:

	FNovaTutorialContract();

	virtual void Initialize(class UNeutronGameInstance* CurrentGameInstance) override;

	virtual TSharedRef<FJsonObject> Save() const override;

	virtual void Load(const TSharedPtr<FJsonObject>& Data) override;

	virtual void OnEvent(const FNeutronContractEvent& Event) override;
};
