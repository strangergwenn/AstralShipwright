// Nova project - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "NovaGameTypes.h"
#include "NovaDestination.generated.h"


/*----------------------------------------------------
	Description types
----------------------------------------------------*/

/** Destination description */
UCLASS(ClassGroup = (Nova))
class UNovaDestination : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	
	// Sub-level to load
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	FName LevelName;

};
