// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "NovaWorldSettings.generated.h"

/** Default game mode class */
UCLASS(ClassGroup = (Nova))
class ANovaWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	ANovaWorldSettings();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

	/** Is the player restricted to menus */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsMenuMap()
	{
		return MenuOnly;
	}

	/** Is the player on the main menu */
	UFUNCTION(Category = Nova, BlueprintCallable)
	bool IsMainMenuMap()
	{
		return MainMenu;
	}

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

	/** Menu mode */
	UPROPERTY(EditDefaultsOnly, NoClear, BlueprintReadOnly, Category = Nova)
	bool MenuOnly;

	/** Main menu mode */
	UPROPERTY(EditDefaultsOnly, NoClear, BlueprintReadOnly, Category = Nova)
	bool MainMenu;
};
