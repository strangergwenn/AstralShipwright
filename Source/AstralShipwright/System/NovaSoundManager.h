// Astral Shipwright - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"

#include "NovaSoundManager.generated.h"

/** Sound & music management class */
UCLASS(ClassGroup = (Nova))
class UNovaSoundManager : public UObject
{
	GENERATED_BODY()

public:
	UNovaSoundManager();

	/*----------------------------------------------------
	    Public methods
	----------------------------------------------------*/

	/** Initialize this class */
	void Initialize(class UNovaGameInstance* Instance);

	/*----------------------------------------------------
	    Public data
	----------------------------------------------------*/
};
