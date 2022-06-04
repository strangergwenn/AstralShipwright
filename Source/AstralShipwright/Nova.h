// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Neutron/Neutron.h"

/*----------------------------------------------------
    Debugging tools
----------------------------------------------------*/

FText GetDateText(struct FNovaTime Time);

FText GetDurationText(struct FNovaTime Time, int32 MaxComponents = 4);

FText GetPriceText(struct FNovaCredits Credits);

/*----------------------------------------------------
    Game module definition
----------------------------------------------------*/

class FNovaModule : public FDefaultGameModuleImpl
{

public:

	void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
	}

	void ShutdownModule() override
	{
		FDefaultGameModuleImpl::ShutdownModule();
	}
};
