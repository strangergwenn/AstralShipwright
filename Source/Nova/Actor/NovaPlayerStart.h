// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"

#include "NovaPlayerStart.generated.h"


/** Player start actor */
UCLASS(ClassGroup = (Nova))
class ANovaPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:

	ANovaPlayerStart(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		// Defaults
		StartDocked = true;
	}


	/*----------------------------------------------------
		Properties
	----------------------------------------------------*/

public:

	// Whether ships here should start docked
	UPROPERTY(Category = Nova, EditAnywhere)
	bool StartDocked;

};
