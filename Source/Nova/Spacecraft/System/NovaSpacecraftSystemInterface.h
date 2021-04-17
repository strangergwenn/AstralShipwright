// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "NovaSpacecraftSystemInterface.generated.h"

/** Interface wrapper */
UINTERFACE(MinimalAPI, Blueprintable)
class UNovaSpacecraftSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/** Base system interface for spacecraft feature components */
class INovaSpacecraftSystemInterface
{
	GENERATED_BODY()

public:
	/*----------------------------------------------------
	    System interface
	----------------------------------------------------*/

	/** Update the simulation between InitialTime and CurrentTime */
	virtual void Update(double InitialTime, double CurrentTime) = 0;
};
