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
	ANovaPlayerStart(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
	{
		// Defaults
		IsInSpace = false;
	}

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

public:
	// Whether ships here should start idle in space
	UPROPERTY(Category = Nova, EditAnywhere)
	bool IsInSpace;
};
